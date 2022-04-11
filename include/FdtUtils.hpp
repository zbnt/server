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
#include <functional>

#include <QString>

extern "C"
{
	#include <libfdt.h>
};

template<typename T1, typename T2>
constexpr T1 *makePointer(T2 *base, uint32_t offset)
{
	return (T1*) (uintptr_t(base) + offset);
}

template<typename T>
void appendAsBytes(QByteArray &array, T data)
{
	array.append((const char*) &data, sizeof(T));
}

template<typename T>
T readAsNumber(const QByteArray &data, quint32 offset)
{
	T res = 0;

	for(uint32_t i = 0; i < sizeof(T); ++i)
	{
		res |= T(quint8(data[offset + i])) << (8 * i);
	}

	return res;
}

extern int64_t getMemoryUsage();
extern bool fdtEnumerateDevices(const void *fdt, int offset, const std::function<bool(const QString&, int, int)> &callback);
extern bool fdtGetStringProp(const void *fdt, int offset, const char *name, QString &out);

template<typename T>
bool fdtArrayToVars(const uint8_t *data, int len, T &out)
{
	out = 0;

	if(len < 4)
	{
		return false;
	}

	for(int i = 0; i < 4; ++i)
	{
		out = (out << 8) | data[i];
	}

	return true;
}

template<typename T, typename... Ts>
bool fdtArrayToVars(const uint8_t *data, int len, T &out, Ts& ...args)
{
	return fdtArrayToVars(data, len, out) && fdtArrayToVars(data + 4, len - 4, args...);
}

template<typename... Ts>
bool fdtGetArrayProp(const void *fdt, int offset, const char *name, Ts& ...args)
{
	int len = 0;
	const void *data = fdt_getprop(fdt, offset, name, &len);

	if(!data)
	{
		return false;
	}

	return fdtArrayToVars((const uint8_t*) data, len, args...);
}
