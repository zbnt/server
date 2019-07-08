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
#include <WorkerThread.hpp>
#include <BitstreamConfig.hpp>
#include <DiscoveryServer.hpp>
#include <MeasurementServer.hpp>

volatile void *axiBase = NULL;

int main(int argc, char **argv)
{
	// Map PL AXI memory block

	int fd = open("/dev/mem", O_RDWR | O_SYNC);

	if(fd == -1)
		return 1;

	axiBase = mmap(NULL, 0xF0000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x43C00000);

	if(!axiBase)
		return EXIT_FAILURE;

	temac[0] = makePointer<uint32_t>(axiBase, 0x00000);
	temac[1] = makePointer<uint32_t>(axiBase, 0x10000);
	temac[2] = makePointer<uint32_t>(axiBase, 0x20000);
	temac[3] = makePointer<uint32_t>(axiBase, 0x30000);

	tgen[0] = makePointer<TrafficGenerator>(axiBase, 0x40000);
	tgen[1] = makePointer<TrafficGenerator>(axiBase, 0x50000);
	tgen[2] = makePointer<TrafficGenerator>(axiBase, 0x60000);
	tgen[3] = makePointer<TrafficGenerator>(axiBase, 0x70000);

	stats[0] = makePointer<StatsCollector>(axiBase, 0x80000);
	stats[1] = makePointer<StatsCollector>(axiBase, 0x90000);
	stats[2] = makePointer<StatsCollector>(axiBase, 0xA0000);
	stats[3] = makePointer<StatsCollector>(axiBase, 0xB0000);

	measurer = makePointer<LatencyMeasurer>(axiBase, 0xC0000);
	timer = makePointer<SimpleTimer>(axiBase, 0xD0000);
	detector = makePointer<FrameDetector>(axiBase, 0xE0000);

	// Program PL

	if(!programPL(BITSTREAM_DUAL_TGEN_LATENCY))
	{
		return EXIT_FAILURE;
	}

	// Initialize Qt application

	QCoreApplication app(argc, argv);

	QThread *workerThreadHandle = QThread::create(workerThread);
	workerThreadHandle->start();
	workerThreadHandle->setPriority(QThread::TimeCriticalPriority);

	new DiscoveryServer();
	new MeasurementServer();

	return app.exec();
}

