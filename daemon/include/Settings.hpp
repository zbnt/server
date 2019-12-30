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

#pragma once

#include <cstdint>

#include <QByteArray>
#include <QString>

enum DaemonModes
{
	MODE_AXI,
	MODE_PCIE
};

struct DaemonConfig
{
	QString deviceName;

	DaemonModes mode;
	uint16_t port;
};

extern DaemonConfig g_daemonCfg;

extern void loadSettings(const char *path, const char *profile);
