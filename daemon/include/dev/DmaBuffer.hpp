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

#include <QAbstractSocket>

#include <dev/AbstractDevice.hpp>

class DmaBuffer : public AbstractDevice
{
public:
	DmaBuffer(const QByteArray &name);
	~DmaBuffer();

	DeviceType getType() const;
	const char *getVirtAddr() const;
	uint64_t getPhysAddr() const;
	size_t getMemSize() const;

	bool isReady() const;
	bool loadDevice(const void *fdt, int offset);

	void appendBuffer(QByteArray &out) const;
	void sendBuffer(QAbstractSocket *thread) const;

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, QByteArray &value);

private:
	uint8_t *m_ptr;
	uint64_t m_physAddr;
	size_t m_memSize;
};

