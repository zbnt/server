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

template<typename T>
constexpr volatile T *makePointer(volatile void *base, uint32_t offset)
{
	return (volatile T*) (uintptr_t(base) + offset);
}

template<typename T>
void appendAsBytes(QByteArray *array, T data)
{
	array->append((const char*) &data, sizeof(T));
}

template<typename T>
T readAsNumber(const QByteArray &data, quint32 offset)
{
	T res = 0;

	for(int i = 0; i < sizeof(T); ++i)
	{
		res |= T(quint8(data[offset + i])) << (8 * i);
	}

	return res;
}

extern void memcpy_v(volatile void *dst, volatile const void *src, uint32_t count);
