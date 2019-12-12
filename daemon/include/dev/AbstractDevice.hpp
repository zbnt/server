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

#include <QByteArray>

enum DeviceType
{
	DEV_AXI_DMA = 1,
	DEV_DMA_BUFFER,
	DEV_SIMPLE_TIMER,
	DEV_FRAME_DETECTOR,
	DEV_STATS_COLLECTOR,
	DEV_LATENCY_MEASURER,
	DEV_TRAFFIC_GENERATOR
};

class AbstractDevice
{
public:
	AbstractDevice(const QByteArray &name);
	virtual ~AbstractDevice();

	virtual DeviceType getType() = 0;
	virtual uint32_t getIdentifier() = 0;

	virtual bool loadDevice(const void *fdt, int offset) = 0;

	virtual void setReset(bool reset) = 0;
	virtual bool setProperty(const QByteArray &key, const QByteArray &value) = 0;
	virtual bool getProperty(const QByteArray &key, QByteArray &value) = 0;

protected:
	QByteArray m_name;
};
