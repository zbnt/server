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

#include <dev/AbstractDevice.hpp>

class DmaBuffer : public AbstractDevice
{
public:
	DmaBuffer(const QByteArray &name);
	~DmaBuffer();

	DeviceType getType();
	uint32_t getIdentifier();

	bool loadDevice(const void *fdt, int offset);

	void setReset(bool reset);
	bool setProperty(const QByteArray &key, const QByteArray &value);
	bool getProperty(const QByteArray &key, QByteArray &value);

private:
	uint8_t *m_ptr;
	size_t m_memSize;
};

