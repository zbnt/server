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
#include <QThread>

#include <Utils.hpp>
#include <Version.hpp>
#include <Settings.hpp>
#include <DiscoveryServer.hpp>
#include <BitstreamManager.hpp>
#include <MeasurementServer.hpp>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	// Load config

	if(argc == 2)
	{
		qInfo("[zbnt] Running version %s", ZBNT_VERSION);
		loadSettings(ZBNT_CFG_PATH "/daemon.cfg", argv[1]);
	}
	else if(argc == 3)
	{
		qInfo("[zbnt] Running version %s", ZBNT_VERSION);
		loadSettings(argv[2], argv[1]);
	}
	else
	{
		qInfo("Usage: zbnt_daemon [profile] [config_file]");
		return 1;
	}

	qInfo("[mem] I: Running in %s mode", g_daemonCfg.mode == MODE_AXI ? "AXI" : "PCIe");

	// Program PL

	initBitstreamManager();

	// Initialize Qt application

	for(const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
	{
		switch(iface.type())
		{
			case QNetworkInterface::Ethernet:
			case QNetworkInterface::Wifi:
			case QNetworkInterface::Virtual:
			{
				g_discoverySrv.append(new DiscoveryServer(iface));
				g_discoverySrv.append(new DiscoveryServer(iface, true));
			}

			default: { }
		}
	}

	g_measurementSrv = new MeasurementServer();

	return app.exec();
}
