/*
	zbnt/server
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

#include <memory>

#include <QCoreApplication>
#include <QThread>

#include <AxiDevice.hpp>
#include <PciDevice.hpp>
#include <Version.hpp>
#include <ZbntTcpServer.hpp>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	// Load config

	if(argc != 2)
	{
		qCritical("Usage: zbnt_server [config]");
		return 1;
	}

	qInfo("[zbnt] Running version %s", ZBNT_VERSION);

	QString configStr = app.arguments()[1].toLower();
	auto config = configStr.splitRef(":");

	if(config[0] != "axi" && config[0] != "pci")
	{
		qCritical("[cfg] F: Invalid mode requested, valid values: AXI, PCI");
		return 1;
	}

	std::unique_ptr<AbstractDevice> dev;
	std::unique_ptr<ZbntServer> server;

	if(config[0] == "axi")
	{
#if defined(__arm__) || defined(__aarch64__)
		if(config.length() > 2)
		{
			qCritical("[cfg] F: Invalid configuration string");
			return 1;
		}

		bool ok = true;
		quint16 port = 0;

		if(config.length() == 2)
		{
			port = config[1].toUShort(&ok);
		}

		if(!ok)
		{
			qCritical("[cfg] F: Invalid port specified");
			return 1;
		}

		dev = std::make_unique<AxiDevice>();
		server = std::make_unique<ZbntTcpServer>(port, dev.get());
#else
		qCritical("[cfg] F: Mode AXI requires running in a Zynq/ZynqMP device");
		return 1;
#endif
	}
	else
	{
		if(config.length() < 3 || config.length() > 4)
		{
			qCritical("[cfg] F: Invalid configuration string");
			return 1;
		}

		if(config[2] == "tcp")
		{
			bool ok = true;
			quint16 port = 0;

			if(config.length() == 4)
			{
				port = config[3].toUShort(&ok);
			}

			if(!ok)
			{
				qCritical("[cfg] F: Invalid port specified");
				return 1;
			}

			dev = std::make_unique<PciDevice>(config[1].toString());
			server = std::make_unique<ZbntTcpServer>(port, dev.get());
		}
		else if(config[2] == "local")
		{
			qCritical("[cfg] F: Local socket mode not yet implemented");
			return 1;
		}
		else
		{
			qCritical("[cfg] F: Invalid socket type requested, valid values: TCP, local");
			return 1;
		}
	}

	return app.exec();
}
