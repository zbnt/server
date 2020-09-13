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

#include <AxiDevice.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>

#include <QDirIterator>

#include <DmaBuffer.hpp>
#include <FdtUtils.hpp>
#include <IrqThread.hpp>
#include <cores/AxiDma.hpp>
#include <cores/SimpleTimer.hpp>

AxiDevice::AxiDevice()
{
	// Enumerate available bitstreams

	QDirIterator it(ZBNT_FIRMWARE_PATH, QDir::Files);

	while(it.hasNext())
	{
		QString binPath = it.next();
		QString binName = it.fileName();

		if(binName.size() <= 4 || !binName.endsWith(".bin"))
		{
			continue;
		}

		QString dtboName = binName.chopped(4) + ".dtbo";
		QString dtboPath = ZBNT_FIRMWARE_PATH "/" + dtboName;

		if(!QFile::exists(dtboPath))
		{
			qWarning("[dev] W: Bitstream \"%s\" lacks a device tree, ignoring.", qUtf8Printable(binName));
			continue;
		}

		m_bitstreamList.append(binName.chopped(4));
	}

	if(!m_bitstreamList.size())
	{
		qFatal("[dev] F: No bitstreams found in " ZBNT_FIRMWARE_PATH);
	}

	std::sort(m_bitstreamList.begin(), m_bitstreamList.end());

	// Create IrqThread

	m_irqThread = new IrqThread(this);

	// Load initial bitstream

	qInfo("[dev] I: %d bitstreams found, using \"%s\" as default.", m_bitstreamList.size(), qUtf8Printable(m_bitstreamList[0]));

	if(!loadBitstream(m_bitstreamList[0]))
	{
		qFatal("[dev] F: Failed to load initial bitstream");
	}
}

AxiDevice::~AxiDevice()
{ }

bool AxiDevice::waitForInterrupt()
{
	uint32_t value;

	pollfd pfd;
	pfd.fd = m_irqfd;
	pfd.events = POLLIN;

	return m_irqfd != -1 && poll(&pfd, 1, 1000) >= 1 && read(m_irqfd, &value, sizeof(value)) == sizeof(value);
}

void AxiDevice::clearInterrupts()
{
	if(m_irqfd != -1)
	{
		uint32_t value = 1;
		write(m_irqfd, &value, sizeof(value));
	}
}

bool AxiDevice::loadBitstream(const QString &name)
{
	qInfo("[dev] I: Loading bitstream: %s", qUtf8Printable(name));

	// Stop IrqThread

	m_irqThread->requestInterruption();
	m_irqThread->wait();

	// Clear devices

	m_uioMap.clear();

	if(m_irqfd != -1)
	{
		close(m_irqfd);
		m_irqfd = -1;
	}

	if(m_dmaEngine)
	{
		delete m_dmaEngine;
		m_dmaEngine = nullptr;
	}

	if(m_timer)
	{
		delete m_timer;
		m_timer = nullptr;
	}

	if(m_dmaBuffer)
	{
		delete m_dmaBuffer;
		m_dmaBuffer = nullptr;
	}

	for(const AbstractCore *core : m_coreList)
	{
		delete core;
	}

	for(auto &m : m_mmapList)
	{
		munmap(m.first, m.second);
	}

	m_coreList.clear();
	m_mmapList.clear();

	// Read dtbo file

	QString dtboPath = ZBNT_FIRMWARE_PATH "/" + name + ".dtbo";
	QFile dtboFile(dtboPath);

	if(!dtboFile.open(QIODevice::ReadOnly))
	{
		qFatal("[dev] F: Failed to open file: %s", qUtf8Printable(dtboPath));
		return false;
	}

	QByteArray dtboContents = dtboFile.readAll();
	const void *fdt = dtboContents.constData();

	// Apply overlay

	if(!loadDeviceTree("zbnt_pl", dtboContents))
	{
		return false;
	}

	// Enumerate UIO devices

	QDirIterator it(ZBNT_SYSFS_PATH "/class/uio", QDir::Dirs | QDir::NoDotAndDotDot);

	while(it.hasNext())
	{
		QString devName = QFileInfo(QFileInfo(it.next() + "/device/of_node").canonicalFilePath()).fileName();
		QByteArray uioName = ("/dev/" + it.fileName()).toUtf8();

		m_uioMap[devName] = uioName;
	}

	// Enumerate devices

	m_activeBitstream = name;

	if(!fdtEnumerateDevices(fdt, 0,
		[&](const QString &name, int offset) -> bool
		{
			QString compatible;

			if(!fdtGetStringProp(fdt, offset, "compatible", compatible))
			{
				return true;
			}

			if(compatible == "ikwzm,u-dma-buf")
			{
				qInfo("[dmabuf] Found %s, type: %s", qUtf8Printable(name), qUtf8Printable(compatible));

				if(m_dmaBuffer)
				{
					qCritical("[dmabuf] E: Multiple DMA buffers found");
					return false;
				}

				// Get properties

				QString devName;

				if(!fdtGetStringProp(fdt, offset, "device-name", devName))
				{
					qCritical("[dmabuf] E: Device tree lacks a valid value for device-name");
					return false;
				}

				size_t size;

				if(!fdtGetArrayProp(fdt, offset, "size", size))
				{
					qCritical("[dmabuf] E: Device tree lacks a valid value for size");
					return false;
				}

				// Get physical address

				QString physAddrPath = ZBNT_SYSFS_PATH "/class/u-dma-buf/";
				physAddrPath.append(devName);
				physAddrPath.append("/phys_addr");

				QFile physAddrFile(physAddrPath);

				if(!physAddrFile.open(QIODevice::ReadOnly))
				{
					qCritical("[dmabuf] E: Failed to open %s", qUtf8Printable(physAddrPath));
					return false;
				}

				bool ok = false;
				uint64_t physAddr = physAddrFile.readAll().toULongLong(&ok, 16);

				if(!ok)
				{
					qCritical("[dmabuf] E: Failed to obtain physical address");
					return false;
				}

				// Open memory map

				QByteArray devPath = "/dev/" + devName.toUtf8();
				int fd = open(devPath.constData(), O_RDONLY);

				if(fd == -1)
				{
					qCritical("[dmabuf] E: Failed to open %s", devPath.constData());
					return false;
				}

				void *buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

				if(!buf || buf == MAP_FAILED)
				{
					qCritical("[dmabuf] E: Failed to mmap %s", devPath.constData());
					close(fd);
					return false;
				}

				m_mmapList.append({buf, size});
				m_dmaBuffer = new DmaBuffer(name, (uint8_t*) buf, physAddr, size);

				close(fd);
			}
			else if(compatible.startsWith("zbnt,"))
			{
				// Find UIO device

				uint32_t id = 0;
				auto it = m_uioMap.constFind(name);

				if(it == m_uioMap.constEnd())
				{
					return true;
				}

				qInfo("[core] Found %s in %s, type: %s", qUtf8Printable(name), it.value().constData(), qUtf8Printable(compatible));

				// Generate ID

				if(compatible == "zbnt,message-dma")
				{
					if(m_dmaEngine)
					{
						qCritical("[core] E: Multiple DMA engines found");
						return false;
					}

					id = 0x100;
				}
				else if(compatible == "zbnt,simple-timer")
				{
					if(m_timer)
					{
						qCritical("[core] E: Multiple timers found");
						return false;
					}

					id = 0xFF;
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

				// Open memory map

				int fd = open(it.value().constData(), O_RDWR | O_SYNC);

				if(fd == -1)
				{
					qCritical("[core] Failed to open UIO device");
					return false;
				}

				void *regs = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

				if(!regs || regs == MAP_FAILED)
				{
					qCritical("[core] E: Failed to mmap UIO device");
					close(fd);
					return false;
				}

				AbstractCore *core = AbstractCore::createCore(this, compatible, name, id, regs, fdt, offset);

				if(!core)
				{
					qCritical("[core] E: Failed to create core");
					munmap(regs, size);
					close(fd);
					return false;
				}

				m_mmapList.append({regs, size});

				switch(core->getType())
				{
					case DEV_AXI_DMA:
					{
						m_dmaEngine = (AxiDma*) core;
						m_irqfd = fd;
						return true;
					}

					case DEV_SIMPLE_TIMER:
					{
						m_timer = (SimpleTimer*) core;
						break;
					}

					default:
					{
						m_coreList.append(core);
						break;
					}
				}

				close(fd);
			}

			return true;
		}
	))
	{
		return false;
	}

	if(!m_dmaEngine)
	{
		qCritical("[core] E: No DMA engine found in device tree");
		return false;
	}

	if(!m_dmaBuffer)
	{
		qCritical("[dmabuf] E: No DMA buffer found in device tree");
		return false;
	}

	if(!m_timer)
	{
		qCritical("[core] E: No timer found in device tree");
		return false;
	}

	// Start IrqThread

	m_irqThread->start();

	return true;
}

const QString &AxiDevice::activeBitstream() const
{
	return m_activeBitstream;
}

const BitstreamList &AxiDevice::bitstreamList() const
{
	return m_bitstreamList;
}

bool AxiDevice::loadDeviceTree(const QString &name, const QByteArray &contents)
{
	QDir configfsDir(ZBNT_CONFIGFS_PATH "/device-tree/overlays");

	if(configfsDir.exists(name))
	{
		const char *dtboDir = qUtf8Printable(configfsDir.absolutePath() + "/" + name);

		rmdir(dtboDir);

		if(configfsDir.exists(name))
		{
			qCritical("[dev] E: Failed to remove device tree overlay %s", qUtf8Printable(name));
			return false;
		}
	}

	configfsDir.mkdir(name);

	QFile dtboDst(ZBNT_CONFIGFS_PATH "/device-tree/overlays/" + name + "/dtbo");

	if(!dtboDst.open(QIODevice::WriteOnly))
	{
		qCritical("[dev] E: Failed to open file: " ZBNT_CONFIGFS_PATH "/device-tree/overlays/%s/dbto", qUtf8Printable(name));
		return false;
	}

	dtboDst.write(contents);
	return true;
}
