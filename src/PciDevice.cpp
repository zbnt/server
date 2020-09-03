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

#include <QDebug>
#include <QDirIterator>

#include <FdtUtils.hpp>

PciDevice::PciDevice(const QString &device)
{
	bool ok = true;
	QString uioPath = "/dev/" + device;
	QString basePath = ZBNT_SYSFS_PATH "/class/uio/" + device;

	// Try to open the UIO device

	m_irqfd = open(uioPath.toUtf8().constData(), O_RDWR | O_SYNC);

	if(m_irqfd == -1)
	{
		qFatal("[dev] F: Can't open device /dev/%s", qUtf8Printable(device));
	}

	// Check the device's name

	QByteArray devName = dumpFile(basePath + "/name").trimmed();

	if(devName != "zbnt:pci")
	{
		qFatal("[dev] F: %s is not a ZBNT device", qUtf8Printable(device));
	}

	// Enumerate memory maps

	for(int i = 0; i < 5; ++i)
	{
		QString mapPath = (basePath + "/maps/map%1").arg(i);

		if(!QFile::exists(mapPath))
		{
			break;
		}

		QByteArray mapName = dumpFile(mapPath + "/name").trimmed();
		size_t mapSize = dumpFile(mapPath + "/size").trimmed().toULong(&ok, 16);

		if(!mapName.size() || !ok)
		{
			continue;
		}

		void *ptr = mmap(NULL, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_irqfd, i*getpagesize());

		if(!ptr || ptr == MAP_FAILED)
		{
			continue;
		}

		m_uioMaps.insert(QString::fromUtf8(mapName), {ptr, mapSize});
	}

	for(const char *r : {"dmabuf_meta", "dmabuf", "static", "rp"})
	{
		if(!m_uioMaps.contains(r))
		{
			qFatal("[dev] F: Missing map %s", qUtf8Printable(device));
		}
	}

	// Create DMA buffer

	auto virtAddr = (uint8_t*) m_uioMaps["dmabuf"].first;
	auto physAddr = *((uint64_t*) m_uioMaps["dmabuf_meta"].first);
	auto size = m_uioMaps["dmabuf"].second;

	m_dmaBuffer = new DmaBuffer("dmabuf0", virtAddr, physAddr, size);

	// Load static partition header

	RegionHeader stHeader;
	memcpy(&stHeader, m_uioMaps["static"].first, sizeof(RegionHeader));

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
	memcpy(dtb.data(), makePointer<void>(m_uioMaps["static"].first, sizeof(RegionHeader)), stHeader.dtb_size);

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

				void *regs = makePointer<void>(m_uioMaps["static"].first, base);
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

	qInfo("[dev] I: Running on board %s, bitstream version: %s", qUtf8Printable(m_boardName), qUtf8Printable(version));

	if(!loadBitstream(m_bitstreamList[0]))
	{
		qFatal("[dev] F: Failed to load initial bitstream");
	}
}

PciDevice::~PciDevice()
{
	if(m_irqfd != -1)
	{
		close(m_irqfd);
	}
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
	memcpy(&rpHeader, m_uioMaps["rp"].first, sizeof(RegionHeader));

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
	memcpy(dtb.data(), makePointer<void>(m_uioMaps["rp"].first, sizeof(RegionHeader)), rpHeader.dtb_size);

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

				void *regs = makePointer<void>(m_uioMaps["rp"].first, base);
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
