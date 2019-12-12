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
#include <DiscoveryServer.hpp>
#include <BitstreamManager.hpp>
#include <MeasurementServer.hpp>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	// Load config

	if(argc <= 1)
	{
		loadSettings(ZBNT_CFG_PATH "/daemon.cfg", "default");
	}
	else if(argc == 2)
	{
		loadSettings(ZBNT_CFG_PATH "/daemon.cfg", argv[1]);
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

	qInfo("[mem] I: Running in %s mode", daemonCfg.mode == MODE_AXI ? "AXI" : "PCIe");

	// Program PL

	initBitstreamManager();

	// Initialize Qt application

	QThread *workerThreadHandle = QThread::create(workerThread);
	workerThreadHandle->start();
	workerThreadHandle->setPriority(QThread::TimeCriticalPriority);

	new DiscoveryServer();
	new MeasurementServer();

	return app.exec();
}
