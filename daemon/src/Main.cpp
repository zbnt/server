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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QCoreApplication>

#include <Utils.hpp>
#include <Settings.hpp>
#include <WorkerThread.hpp>
#include <BitstreamConfig.hpp>
#include <DiscoveryServer.hpp>
#include <MeasurementServer.hpp>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	// Load config

	if(argc <= 1)
	{
		loadSettings(ZBNT_DAEMON_CFG_PATH "/daemon.cfg", "default");
	}
	else if(argc == 2)
	{
		loadSettings(ZBNT_DAEMON_CFG_PATH "/daemon.cfg", argv[1]);
	}
	else if(argc == 3)
	{
		loadSettings(argv[2], argv[1]);
	}
	else
	{
		qInfo("Usage: zbnt_daemon [profile] [config_file]");
		return 1;
	}

	qInfo("[mem] Mode: %s", daemonCfg.mode == MODE_AXI ? "AXI" : "PCIe");
	qInfo("[mem] Device: %s", qUtf8Printable(daemonCfg.memDevice));
	qInfo("[mem] Base address: 0x%llX", daemonCfg.memBase);

	if(daemonCfg.mode == MODE_PCIE)
		qFatal("[mem] PCIe mode not yet implemented");

	// Map memory block

	int fd = open(daemonCfg.memDevice.toLocal8Bit().data(), O_RDWR | O_SYNC);

	if(fd == -1)
		qFatal("[mem] open() returned -1");

	volatile void *memBase = mmap(NULL, 0xD0000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, daemonCfg.memBase);

	if(!memBase)
		qFatal("[mem] mmap() returned NULL");

	timer = makePointer<SimpleTimer>(memBase, 0x00000);

	stats[0] = makePointer<StatsCollector>(memBase, 0x10000);
	stats[1] = makePointer<StatsCollector>(memBase, 0x20000);
	stats[2] = makePointer<StatsCollector>(memBase, 0x30000);
	stats[3] = makePointer<StatsCollector>(memBase, 0x40000);

	tgen[0] = makePointer<TrafficGenerator>(memBase, 0x50000);
	tgen[1] = makePointer<TrafficGenerator>(memBase, 0x60000);
	tgen[2] = makePointer<TrafficGenerator>(memBase, 0x70000);
	tgen[3] = makePointer<TrafficGenerator>(memBase, 0x80000);

	measurer = makePointer<LatencyMeasurer>(memBase, 0x90000);
	detector = makePointer<FrameDetector>(memBase, 0xB0000);

	// Program PL

	if(!programPL(BITSTREAM_QUAD_TGEN))
		qDebug("[prog] Failed to program initial bitstream");

	// Initialize Qt application

	QThread *workerThreadHandle = QThread::create(workerThread);
	workerThreadHandle->start();
	workerThreadHandle->setPriority(QThread::TimeCriticalPriority);

	new DiscoveryServer();
	new MeasurementServer();

	return app.exec();
}
