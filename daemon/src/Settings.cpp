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

DaemonConfig daemonCfg;

void loadSettings(const char *path, const char *profile)
{
	QSettings cfg(path, QSettings::IniFormat);
	cfg.beginGroup(profile);

	qInfo("[cfg] Loading %s", path);
	qInfo("[cfg] Active profile: %s", profile);

	if(!QFile::exists(path))
		qFatal("[cfg] Configuration file does not exist");

	// Check for required keys

	for(auto k : {"mode", "mem_dev", "mem_base"})
	{
		if(!cfg.contains(k))
			qFatal("[cfg] Missing configuration key: %s", k);
	}

	//

	QString mode = cfg.value("mode").toString().toLower();

	if(mode != "pcie" && mode != "axi")
		qFatal("[cfg] Invalid value for type, valid values: axi, pcie");

	daemonCfg.mode = (mode == "axi") ? MODE_AXI : MODE_PCIE;

	//

	daemonCfg.memDevice = cfg.value("mem_dev").toString();

	if(!QFile::exists(daemonCfg.memDevice))
		qFatal("[cfg] Invalid value for mem_dev, file does not exist");

	//

	bool ok = false;
	daemonCfg.memBase = cfg.value("mem_base").toString().toULongLong(&ok, 16);

	if(!ok)
		qFatal("[cfg] Invalid value for mem_base");

	//

	daemonCfg.mainPort = 5464;

	if(cfg.contains("main_port"))
	{
		quint16 mainPort = cfg.value("main_port").toString().toUShort(&ok);

		if(ok)
		{
			daemonCfg.mainPort = mainPort;
		}
		else
		{
			qWarning("[cfg] Invalid value for main_port, using default value: 5464");
		}
	}

	//

	daemonCfg.streamPort = 5465;

	if(cfg.contains("stream_port"))
	{
		quint16 streamPort = cfg.value("stream_port").toString().toUShort(&ok);

		if(ok)
		{
			daemonCfg.streamPort = streamPort;
		}
		else
		{
			qWarning("[cfg] Invalid value for stream_port, using default value: 5465");
		}
	}
}
