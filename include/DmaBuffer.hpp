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

#include <QString>

class DmaBuffer
{
public:
	DmaBuffer(const QString &name, uint8_t *virtAddr, uint64_t physAddr, size_t size);
	~DmaBuffer();

	uint8_t *getVirtualAddr() const;
	uint64_t getPhysicalAddr() const;
	size_t getSize() const;

private:
	QString m_name;
	uint8_t *m_virtAddr;
	uint64_t m_physAddr;
	size_t m_memSize;
};

