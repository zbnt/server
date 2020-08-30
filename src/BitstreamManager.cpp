/*
	zbnt_sw
	Copyright (C) 2019 Oscar R.

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

#include <BitstreamManager.hpp>

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QDirIterator>

#include <unistd.h>
#include <sys/mman.h>

#include <FdtUtils.hpp>
#include <Settings.hpp>
#include <dev/AxiDma.hpp>
#include <dev/AxiMdio.hpp>
#include <dev/DmaBuffer.hpp>
#include <dev/SimpleTimer.hpp>
#include <dev/FrameDetector.hpp>
#include <dev/AbstractDevice.hpp>
#include <dev/StatsCollector.hpp>
#include <dev/LatencyMeasurer.hpp>
#include <dev/TrafficGenerator.hpp>

QString g_activeBitstream;
QVector<QString> g_bitstreamList;
QHash<QByteArray, QByteArray> g_uioMap;

AxiDma *g_axiDma = nullptr;
DmaBuffer *g_dmaBuffer = nullptr;
SimpleTimer *g_axiTimer = nullptr;
QVector<AbstractDevice*> g_deviceList;

void initBitstreamManager()
{
	QDirIterator it(ZBNT_FIRMWARE_PATH "/" + g_daemonCfg.deviceName, QDir::Files);

	while(it.hasNext())
	{
		QString binPath = it.next();
		QString binName = it.fileName();

		if(binName.size() <= 4 || !binName.endsWith(".bin"))
		{
			continue;
		}

		QString dtboName = binName.chopped(4) + ".dtbo";
		QString dtboPath = ZBNT_FIRMWARE_PATH "/" + g_daemonCfg.deviceName + "/" + dtboName;

		if(!QFile::exists(dtboPath))
		{
			qWarning("[bitmgr] W: Bitstream \"%s\" lacks a device tree, ignoring.", qUtf8Printable(binName));
			continue;
		}

		g_bitstreamList.append(binName.chopped(4));
	}

	if(!g_bitstreamList.size())
	{
		qFatal("[bitmgr] F: No bitstreams found in " ZBNT_FIRMWARE_PATH);
	}

	std::sort(g_bitstreamList.begin(), g_bitstreamList.end());

	qInfo("[bitmgr] I: %d bitstreams found, using \"%s\" as default.", g_bitstreamList.size(), qUtf8Printable(g_bitstreamList[0]));

	if(!loadBitstream(g_bitstreamList[0]))
	{
		qFatal("[bitmgr] F: Failed to load initial bitstream.");
	}
}

bool loadBitstream(const QString &bitstreamName)
{
	qInfo("[bitmgr] I: Loading bitstream: %s", qUtf8Printable(bitstreamName));

	// Clear devices

	g_uioMap.clear();

	if(g_axiDma)
	{
		delete g_axiDma;
		g_axiDma = nullptr;
	}

	if(g_axiTimer)
	{
		delete g_axiTimer;
		g_axiTimer = nullptr;
	}

	if(g_dmaBuffer)
	{
		delete g_dmaBuffer;
		g_dmaBuffer = nullptr;
	}

	for(const AbstractDevice *dev : g_deviceList)
	{
		delete dev;
	}

	g_deviceList.clear();

	// Read dtbo file

	QString dtboPath = ZBNT_FIRMWARE_PATH "/" + g_daemonCfg.deviceName + "/" + bitstreamName + ".dtbo";
	QFile dtboFile(dtboPath);

	if(!dtboFile.open(QIODevice::ReadOnly))
	{
		qFatal("[bitmgr] F: Failed to open file: %s", qUtf8Printable(dtboPath));
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
		QByteArray devName = QFileInfo(QFileInfo(it.next() + "/device/of_node").canonicalFilePath()).fileName().toUtf8();
		QByteArray uioName = it.fileName().toUtf8();

		g_uioMap[devName] = uioName;
	}

	// Enumerate devices

	g_activeBitstream = bitstreamName;

	return fdtEnumerateDevices(fdt, 0,
		[&](const QByteArray &name, int offset)
		{
			if(name == "udmabuf")
			{
				g_dmaBuffer = new DmaBuffer(name);

				if(!g_dmaBuffer || !g_dmaBuffer->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create DmaBuffer instance for %s", name.constData());
					return false;
				}
			}
			else if(name.startsWith("dma@"))
			{
				g_axiDma = new AxiDma(name);

				if(!g_axiDma || !g_axiDma->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create AxiDma instance for %s", name.constData());
					return false;
				}
			}
			else if(name.startsWith("zbnt_timer@"))
			{
				g_axiTimer = new SimpleTimer(name);

				if(!g_axiTimer || !g_axiTimer->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create SimpleTimer instance for %s", name.constData());
					return false;
				}
			}
			else if(name.startsWith("axi_mdio@"))
			{
				AxiMdio *dev = new AxiMdio(name, g_deviceList.size());

				if(!dev || !dev->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create AxiMdio instance for %s", name.constData());
					return false;
				}

				g_deviceList.append(dev);
			}
			else if(name.startsWith("zbnt_fd@"))
			{
				FrameDetector *dev = new FrameDetector(name, g_deviceList.size());

				if(!dev || !dev->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create FrameDetector instance for %s", name.constData());
					return false;
				}

				g_deviceList.append(dev);
			}
			else if(name.startsWith("zbnt_sc@"))
			{
				StatsCollector *dev = new StatsCollector(name, g_deviceList.size());

				if(!dev || !dev->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create StatsCollector instance for %s", name.constData());
					return false;
				}

				g_deviceList.append(dev);
			}
			else if(name.startsWith("zbnt_lm@"))
			{
				LatencyMeasurer *dev = new LatencyMeasurer(name, g_deviceList.size());

				if(!dev || !dev->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create LatencyMeasurer instance for %s", name.constData());
					return false;
				}

				g_deviceList.append(dev);
			}
			else if(name.startsWith("zbnt_tg@"))
			{
				TrafficGenerator *dev = new TrafficGenerator(name, g_deviceList.size());

				if(!dev || !dev->loadDevice(fdt, offset))
				{
					qCritical("[bitmgr] E: Failed to create TrafficGenerator instance for %s", name.constData());
					return false;
				}

				g_deviceList.append(dev);
			}

			return true;
		}
	);
}

bool loadDeviceTree(const QString &dtboName, const QByteArray &dtboContents)
{
	QDir configfsDir(ZBNT_CONFIGFS_PATH "/device-tree/overlays");

	if(configfsDir.exists(dtboName))
	{
		const char *dtboDir = qUtf8Printable(configfsDir.absolutePath() + "/" + dtboName);

		rmdir(dtboDir);

		if(configfsDir.exists(dtboName))
		{
			qCritical("[bitmgr] E: Failed to remove device tree overlay %s", qUtf8Printable(dtboName));
			return false;
		}
	}

	configfsDir.mkdir(dtboName);

	QFile dtboDst(ZBNT_CONFIGFS_PATH "/device-tree/overlays/" + dtboName + "/dtbo");

	if(!dtboDst.open(QIODevice::WriteOnly))
	{
		qCritical("[bitmgr] E: Failed to open file: " ZBNT_CONFIGFS_PATH "/device-tree/overlays/%s/dbto", qUtf8Printable(dtboName));
		return false;
	}

	dtboDst.write(dtboContents);
	return true;
}
