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
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

#include <AxiDevice.hpp>
#include <PciDevice.hpp>
#include <CfgUtils.hpp>
#include <Version.hpp>
#include <ZbntTcpServer.hpp>
#include <ZbntLocalServer.hpp>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	if(argc != 2)
	{
		qCritical("Usage: zbnt_server [profile]");
		return 1;
	}

	// Open profile configuration file

	QString profile = argv[1];

	if(!profile.length() || profile.indexOf('/') != -1)
	{
		qCritical("[cfg] F: Invalid profile name");
		return 1;
	}

	QString cfgFile(ZBNT_PROFILE_PATH "/" + profile + ".cfg");

	if(!QFileInfo::exists(cfgFile))
	{
		qCritical("[cfg] F: Configuration file not found: %s", qUtf8Printable(cfgFile));
		return 1;
	}

	QSettings settings(ZBNT_PROFILE_PATH "/" + profile + ".cfg", QSettings::IniFormat);

	qInfo("[zbnt] Running version %s", ZBNT_VERSION);
	qInfo("[cfg] Using settings in file %s", qUtf8Printable(cfgFile));

	// Load device-related settings

	std::unique_ptr<AbstractDevice> dev;

#if ZBNT_ZYNQ_MODE
	dev = std::make_unique<AxiDevice>();
#else
	qInfo("[cfg] Loading device settings");

	settings.beginGroup("device");

	QString slot;
	readSetting(settings, "pci-slot", slot);
	slot = slot.toLower();

	if(!slot.contains(QRegularExpression("^[0-9a-f]{4}(?:\\:[0-9a-f]{2}){2}\\.[0-9]$")))
	{
		qCritical("[cfg] F: Invalid value for setting: pci-slot");
		return 1;
	}

	dev = std::make_unique<PciDevice>(slot);

	settings.endGroup();
#endif

	// Load server-related settings

	std::unique_ptr<ZbntServer> server;

	qInfo("[cfg] Loading server settings");

	settings.beginGroup("server");

	QString type;
	readSetting(settings, "type", type);
	type = type.toLower();

	if(type == "tcp")
	{
		QString address;
		quint16 port;

		readSetting(settings, "address", address, QString("::"));
		readSetting(settings, "port", port, quint16(0));

		server = std::make_unique<ZbntTcpServer>(QHostAddress(address), port, dev.get());
	}
	else if(type == "local")
	{
		QString name;

		readSetting(settings, "name", name);

		server = std::make_unique<ZbntLocalServer>(name, dev.get());
	}
	else
	{
		qCritical("[cfg] F: Invalid value for setting: type");
		return 1;
	}

	settings.endGroup();

	return app.exec();
}
