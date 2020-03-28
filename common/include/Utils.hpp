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
#include <functional>

#include <QString>
#include <QIODevice>
#include <QByteArray>

#include <Messages.hpp>

template<typename T1, typename T2>
constexpr T1 *makePointer(T2 *base, uint32_t offset)
{
	return (T1*) (uintptr_t(base) + offset);
}

template<typename T>
void writeAsBytes(QIODevice *dev, T data)
{
	dev->write((const char*) &data, sizeof(T));
}

template<typename T>
void appendAsBytes(QByteArray &array, T data)
{
	array.append((const char*) &data, sizeof(T));
}

template<typename T>
T readAsNumber(const QByteArray &data, uint32_t offset)
{
	T res = 0;

	for(uint32_t i = 0; i < sizeof(T); ++i)
	{
		res |= T(uint8_t(data[offset + i])) << (8 * i);
	}

	return res;
}

template<typename T>
void buildMessage(QByteArray &array, MessageID msgID, T value)
{
	array.append(MSG_MAGIC_IDENTIFIER, 4);
	appendAsBytes<uint16_t>(array, msgID);
	appendAsBytes<uint16_t>(array, sizeof(T));
	appendAsBytes(array, value);
}

template<typename T>
void setDeviceProperty(QByteArray &array, uint8_t devID, PropertyID propID, T value)
{
	array.append(MSG_MAGIC_IDENTIFIER, 4);
	appendAsBytes<uint16_t>(array, MSG_ID_SET_PROPERTY);
	appendAsBytes<uint16_t>(array, 3 + sizeof(T));
	appendAsBytes<uint8_t>(array, devID);
	appendAsBytes<uint16_t>(array, propID);
	appendAsBytes(array, value);
}

extern void writeMessage(QIODevice *dev, MessageID msgID, const QByteArray &value);
extern void buildMessage(QByteArray &array, MessageID msgID, const QByteArray &value);
extern void setDeviceProperty(QByteArray &array, uint8_t devID, PropertyID propID, const QByteArray &value);

extern int64_t getMemoryUsage();

extern void padByteArray(QByteArray &array, int length, char padding = 0);
extern QByteArray padString(const char *str, int length, char padding = 0);

extern void cyclesToTime(uint64_t cycles, QString &time);
extern void bytesToHumanReadable(uint64_t bytes, QString &res, bool dec = false);
extern void bitsToHumanReadable(uint64_t bytes, QString &res, bool dec = false);
