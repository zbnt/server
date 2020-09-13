/*
	zbnt/server
	Copyright (C) 2020 Oscar R.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <PciDevice.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <linux/vfio.h>

#include <QDebug>
#include <QDirIterator>

#include <FdtUtils.hpp>
#include <IrqThread.hpp>

PciDevice::PciDevice(const QString &device)
{
	// Get IOMMU group for device

	QFileInfo groupLink = "/sys/bus/pci/devices/" + device + "/iommu_group";

	if(!groupLink.exists())
	{
		qFatal("[dev] F: No such PCI device: %s", qUtf8Printable(device));
	}

	bool ok = true;
	int group = QFileInfo(groupLink.symLinkTarget()).fileName().toInt(&ok);

	if(!ok)
	{
		qFatal("[dev] F: Can't get IOMMU group for device: %s", qUtf8Printable(device));
	}

	qInfo("[dev] I: Device %s is part of IOMMU group %d", qUtf8Printable(device), group);

	// Create VFIO container

	m_container = open("/dev/vfio/vfio", O_RDWR);

	if(m_container == -1)
	{
		qFatal("[dev] F: Can't open /dev/vfio/vfio");
	}

	if(ioctl(m_container, VFIO_GET_API_VERSION) != VFIO_API_VERSION)
	{
		qFatal("[dev] F: Unknown VFIO API version");
	}

	if(!ioctl(m_container, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU))
	{
		qFatal("[dev] F: No support for VFIO type 1");
	}

	// Open VFIO group

	QString groupPath = QString("/dev/vfio/%1").arg(group);
	m_group = open(groupPath.toUtf8().data(), O_RDWR);

	if(m_group == -1)
	{
		qFatal("[dev] F: Can't open /dev/vfio/%d", group);
	}

	vfio_group_status groupStatus = {sizeof(groupStatus)};
	ioctl(m_group, VFIO_GROUP_GET_STATUS, &groupStatus);

	if(!(groupStatus.flags & VFIO_GROUP_FLAGS_VIABLE))
	{
		qFatal("[dev] F: Group %d is not ready", group);
	}

	ioctl(m_group, VFIO_GROUP_SET_CONTAINER, &m_container);
	ioctl(m_container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);

	m_device = ioctl(m_group, VFIO_GROUP_GET_DEVICE_FD, device.toUtf8().data());

	// Enumerate memory maps

	vfio_device_info devInfo = {sizeof(devInfo)};
	ioctl(m_device, VFIO_DEVICE_GET_INFO, &devInfo);

	for(uint32_t i = 0; i < devInfo.num_regions; ++i)
	{
		vfio_region_info regInfo = {sizeof(regInfo)};
		regInfo.index = i;

		ioctl(m_device, VFIO_DEVICE_GET_REGION_INFO, &regInfo);

		if(regInfo.flags & VFIO_REGION_INFO_FLAG_MMAP)
		{
			void *ptr = mmap(NULL, regInfo.size, PROT_READ | PROT_WRITE, MAP_SHARED, m_device, regInfo.offset);

			if(!ptr || ptr == MAP_FAILED)
			{
				continue;
			}

			qInfo("[dev] I: Found region %u with size %llu KiB", regInfo.index, regInfo.size / 1024);

			m_memMaps.append({ptr, size_t(regInfo.size)});
		}

		if(i == VFIO_PCI_CONFIG_REGION_INDEX)
		{
			m_confRegion = regInfo.offset;
		}
	}

	// Load static partition header

	RegionHeader stHeader;
	memcpy(&stHeader, m_memMaps[0].first, sizeof(RegionHeader));

	if(memcmp(stHeader.magic, "ZBNT\x00", 5))
	{
		qFatal("[dev] F: Static partition has invalid magic header");
	}

	if(memcmp(stHeader.type, "ST\x00", 3))
	{
		qFatal("[dev] F: Static partition has invalid type in header");
	}

	QString version = QString("%1.%2.%3").arg(stHeader.version_base >> 24)
	                                     .arg((stHeader.version_base >> 16) & 0xFF)
	                                     .arg(stHeader.version_base & 0xFFFF);

	if(stHeader.version_prerel[0])
	{
		version.append("-");
		version.append(stHeader.version_prerel);
	}

	if(stHeader.version_commit[0])
	{
		version.append("+");
		version.append(stHeader.version_commit);

		if(stHeader.version_dirty)
		{
			version.append(".d");
		}
	}
	else if(stHeader.version_dirty)
	{
		version.append("+d");
	}

	QByteArray dtb(stHeader.dtb_size, 0);
	memcpy(dtb.data(), makePointer<void>(m_memMaps[0].first, sizeof(RegionHeader)), stHeader.dtb_size);

	// Create DMA buffer

	vfio_iommu_type1_dma_map dmaMap;

	dmaMap.argsz = sizeof(dmaMap);
	dmaMap.vaddr = uint64_t(mmap(NULL, 0xA00000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	dmaMap.size  = 0xA00000;
	dmaMap.iova  = 0xA00000;
	dmaMap.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;

	if((void*) dmaMap.vaddr == MAP_FAILED)
	{
		qFatal("[dmabuf] F: Failed to allocate DMA buffer");
	}

	ioctl(m_container, VFIO_IOMMU_MAP_DMA, &dmaMap);

	m_dmaBuffer = new DmaBuffer("dmabuf0", (uint8_t*) dmaMap.vaddr, dmaMap.iova, dmaMap.size);

	// Setup interrupt handler

	uint8_t *setIrqMem = new uint8_t[sizeof(vfio_irq_set) + sizeof(int)];

	if(!setIrqMem)
	{
		qFatal("[dev] Can't allocate vfio_irq_set");
	}

	vfio_irq_set *setIrq = (vfio_irq_set*) setIrqMem;
	int *setIrqFd = (int*) (setIrqMem + sizeof(vfio_irq_set));

	setIrq->argsz = sizeof(vfio_irq_set) + sizeof(int);
	setIrq->flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER;
	setIrq->index = VFIO_PCI_MSI_IRQ_INDEX;
	setIrq->start = 0;
	setIrq->count = 1;

	m_irqfd = eventfd(0, 0);
	*setIrqFd = m_irqfd;

	ioctl(m_device, VFIO_DEVICE_SET_IRQS, setIrq);
	delete [] setIrqMem;

	// Enumerate devices in static partition

	const char *fdt = dtb.constData();

	fdtGetStringProp(fdt, 0, "compatible", m_boardName);
	m_boardName = m_boardName.mid(5);

	if(!fdtEnumerateDevices(fdt, 0,
		[&](const QString &name, int offset) -> bool
		{
			QString compatible;

			if(!fdtGetStringProp(fdt, offset, "compatible", compatible))
			{
				return true;
			}

			if(compatible.startsWith("zbnt,") && compatible != "zbnt,dtb-rom")
			{
				qInfo("[core] Found %s in static partition, type: %s", qUtf8Printable(name), qUtf8Printable(compatible));

				// Generate ID

				if(compatible == "zbnt,message-dma")
				{
					if(m_dmaEngine)
					{
						qCritical("[core] E: Multiple DMA engines found");
						return false;
					}
				}
				else if(compatible == "zbnt,pr-controller")
				{
					if(m_prCtl)
					{
						qCritical("[core] E: Multiple partial reconfiguration controllers found");
						return false;
					}
				}
				else
				{
					return true;
				}

				// Get memory range

				uintptr_t base;
				size_t size;

				if(!fdtGetArrayProp(fdt, offset, "reg", base, size))
				{
					qCritical("[core] E: Device tree lacks a valid value for reg");
					return false;
				}

				// Create core

				void *regs = makePointer<void>(m_memMaps[0].first, base);
				AbstractCore *core = AbstractCore::createCore(this, compatible, name, 0, regs, fdt, offset);

				if(!core)
				{
					qCritical("[core] E: Failed to create core");
					return false;
				}

				switch(core->getType())
				{
					case DEV_AXI_DMA:
					{
						m_dmaEngine = (AxiDma*) core;
						return true;
					}

					case DEV_PR_CONTROLLER:
					{
						m_prCtl = (PrController*) core;
						m_bitstreamList = m_prCtl->bitstreamList();
						break;
					}

					default:
					{
						return false;
					}
				}
			}

			return true;
		}
	))
	{
		qFatal("[core] F: Failed to enumerate cores in static partition");
	}

	if(!m_dmaEngine)
	{
		qFatal("[core] E: No DMA engine found in device tree");
	}

	if(!m_prCtl)
	{
		qFatal("[core] E: No PR controller found in device tree");
	}

	// Load initial bitstream

	qInfo("[dev] I: Running on board %s, static bitstream version: %s", qUtf8Printable(m_boardName), qUtf8Printable(version));

	if(!loadBitstream(m_bitstreamList[0]))
	{
		qFatal("[dev] F: Failed to load initial bitstream");
	}

	// Create and start IrqThread

	m_irqThread = new IrqThread(this);
	m_irqThread->start();

	// Enable DMA

	uint16_t pciCommand;
	pread(m_device, &pciCommand, sizeof(pciCommand), m_confRegion + 0x4);

	pciCommand |= 0b100;
	pwrite(m_device, &pciCommand, sizeof(pciCommand), m_confRegion + 0x4);
}

PciDevice::~PciDevice()
{
	for(auto &mm : m_memMaps)
	{
		munmap(mm.first, mm.second);
	}

	if(m_group != -1)
	{
		close(m_group);
	}

	if(m_container != -1)
	{
		close(m_container);
	}
}

bool PciDevice::waitForInterrupt()
{
	uint64_t value;
	return m_irqfd != -1 && read(m_irqfd, &value, sizeof(value)) == sizeof(value);
}

void PciDevice::clearInterrupts()
{
	// no-op
}

bool PciDevice::loadBitstream(const QString &name)
{
	qInfo("[dev] I: Loading bitstream: %s", qUtf8Printable(name));

	// Trigger the PR controller

	if(!m_prCtl->loadBitstream(name))
	{
		qCritical("[dev] E: PR controller failed with status: 0x%08X", m_prCtl->status());
		return false;
	}

	// Clear devices

	if(m_timer)
	{
		delete m_timer;
		m_timer = nullptr;
	}

	for(const AbstractCore *core : m_coreList)
	{
		delete core;
	}

	m_coreList.clear();

	// Load RP header

	RegionHeader rpHeader;
	memcpy(&rpHeader, m_memMaps[1].first, sizeof(RegionHeader));

	if(memcmp(rpHeader.magic, "ZBNT\x00", 5))
	{
		qCritical("[dev] E: Reconfigurable partition has invalid magic header");
		return false;
	}

	if(memcmp(rpHeader.type, "RP\x00", 3))
	{
		qCritical("[dev] E: Reconfigurable partition has invalid type in header");
		return false;
	}

	QByteArray dtb(rpHeader.dtb_size, 0);
	memcpy(dtb.data(), makePointer<void>(m_memMaps[1].first, sizeof(RegionHeader)), rpHeader.dtb_size);

	// Enumerate devices in reconfigurable partition

	int id = 0;
	const char *fdt = dtb.constData();

	if(!fdtEnumerateDevices(fdt, 0,
		[&](const QString &name, int offset) -> bool
		{
			QString compatible;

			if(!fdtGetStringProp(fdt, offset, "compatible", compatible))
			{
				return true;
			}

			if(compatible.startsWith("zbnt,") && compatible != "zbnt,rp_dtb")
			{
				qInfo("[core] Found %s in reconfigurable partition, type: %s", qUtf8Printable(name), qUtf8Printable(compatible));

				// Generate ID

				if(compatible == "zbnt,simple-timer")
				{
					if(m_timer)
					{
						qCritical("[core] E: Multiple timers found");
						return false;
					}
				}
				else
				{
					id = m_coreList.size();
				}

				// Get memory range

				uintptr_t base;
				size_t size;

				if(!fdtGetArrayProp(fdt, offset, "reg", base, size))
				{
					qCritical("[core] E: Device tree lacks a valid value for reg");
					return false;
				}

				// Create core

				void *regs = makePointer<void>(m_memMaps[1].first, base);
				AbstractCore *core = AbstractCore::createCore(this, compatible, name, id, regs, fdt, offset);

				if(!core)
				{
					qCritical("[core] E: Failed to create core");
					return false;
				}

				if(core->getType() == DEV_SIMPLE_TIMER)
				{
					m_timer = (SimpleTimer*) core;
				}
				else
				{
					m_coreList.append(core);
				}
			}

			return true;
		}
	))
	{
		return false;
	}

	if(!m_timer)
	{
		qCritical("[core] E: No timer found in device tree");
		return false;
	}

	return true;
}

const QString &PciDevice::activeBitstream() const
{
	return m_bitstreamList[m_prCtl->activeBitstream()];
}

const BitstreamList &PciDevice::bitstreamList() const
{
	return m_bitstreamList;
}
