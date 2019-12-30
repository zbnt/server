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

#include <Settings.hpp>

#include <QFile>
#include <QDebug>
#include <QVariant>
#include <QSettings>

DaemonConfig g_daemonCfg;

void loadSettings(const char *path, const char *profile)
{
	if(!QFile::exists(path))
		qFatal("[cfg] F: Configuration file does not exist.");

	QSettings cfg(path, QSettings::IniFormat);
	cfg.beginGroup(profile);

	qInfo("[cfg] I: Loading %s", path);
	qInfo("[cfg] I: Active profile: %s", profile);

	// Check for required keys

	for(auto k : {"mode"})
	{
		if(!cfg.contains(k))
			qFatal("[cfg] F: Missing configuration key: %s", k);
	}

	//

	QString mode = cfg.value("mode").toString().toLower();

	if(mode != "pcie" && mode != "axi")
	{
		qFatal("[cfg] F: Invalid value for type, valid values: AXI, PCIe");
	}

	g_daemonCfg.mode = (mode == "axi") ? MODE_AXI : MODE_PCIE;

	//

	bool ok = false;
	g_daemonCfg.port = -1;

	if(cfg.contains("listenPort"))
	{
		int32_t port = cfg.value("listenPort").toString().toInt(&ok);

		if(ok && port >= -1 && port <= 65535)
		{
			g_daemonCfg.port = port;
		}
		else
		{
			qWarning("[cfg] W: Invalid value for listenPort, using default value: -1");
		}
	}

	g_daemonCfg.deviceName = profile;
}
