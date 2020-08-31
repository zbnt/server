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

#include <QByteArray>

#include <Messages.hpp>

class AbstractCore
{
public:
	AbstractCore(const QByteArray &name, uint32_t index);
	virtual ~AbstractCore();

	virtual void announce(QByteArray &output) const = 0;

	const QByteArray &getName() const;
	virtual DeviceType getType() const = 0;
	virtual uint32_t getIndex() const;
	virtual uint64_t getPorts() const;

	virtual bool isReady() const = 0;
	virtual bool loadDevice(const void *fdt, int offset) = 0;

	virtual void setReset(bool reset) = 0;
	virtual bool setProperty(PropertyID propID, const QByteArray &value) = 0;
	virtual bool getProperty(PropertyID propID, const QByteArray &params, QByteArray &value) = 0;

protected:
	QByteArray m_name;
	uint32_t m_idx;
};
