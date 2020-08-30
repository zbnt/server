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

#pragma once

#include <cstdint>

#include <QHash>
#include <QVector>

#include <dev/AxiDma.hpp>
#include <dev/DmaBuffer.hpp>
#include <dev/SimpleTimer.hpp>

extern QString g_activeBitstream;
extern QVector<QString> g_bitstreamList;
extern QHash<QByteArray, QByteArray> g_uioMap;

extern AxiDma *g_axiDma;
extern DmaBuffer *g_dmaBuffer;
extern SimpleTimer *g_axiTimer;
extern QVector<AbstractDevice*> g_deviceList;

void initBitstreamManager();
bool loadBitstream(const QString &bitstreamName);
bool loadDeviceTree(const QString &dtboName, const QByteArray &dtboContents);
