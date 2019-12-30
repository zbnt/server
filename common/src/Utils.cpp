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

#include <Utils.hpp>

#include <vector>

#include <QFile>
#include <QVector>
#include <QTextStream>

void writeMessage(QIODevice *dev, MessageID msgID, const QByteArray &value)
{
	dev->write(MSG_MAGIC_IDENTIFIER, 4);
	writeAsBytes<uint16_t>(dev, msgID);
	writeAsBytes<uint16_t>(dev, value.size());
	dev->write(value);
}

void buildMessage(QByteArray &array, MessageID msgID, const QByteArray &value)
{
	array.append(MSG_MAGIC_IDENTIFIER, 4);
	appendAsBytes<uint16_t>(array, msgID);
	appendAsBytes<uint16_t>(array, value.size());
	array.append(value);
}

void setDeviceProperty(QByteArray &array, uint8_t devID, PropertyID propID, const QByteArray &value)
{
	array.append(MSG_MAGIC_IDENTIFIER, 4);
	appendAsBytes<uint16_t>(array, MSG_ID_SET_PROPERTY);
	appendAsBytes<uint16_t>(array, 3 + value.size());
	appendAsBytes<uint8_t>(array, devID);
	appendAsBytes<uint16_t>(array, propID);
	array.append(value);
}

int64_t getMemoryUsage()
{
	QFile statusFile("/proc/self/status");

	if(statusFile.open(QIODevice::ReadOnly))
	{
		QTextStream statusStream(&statusFile);
		QString line;

		while(!(line = statusStream.readLine()).isNull())
		{
			QVector<QStringRef> lineFields = line.splitRef(QRegExp("\\s+"), QString::SkipEmptyParts);

			if(lineFields.size() == 3 && lineFields[0] == "VmSize:")
			{
				bool ok = false;
				int64_t res = lineFields[1].toLongLong(&ok);

				if(ok)
				{
					for(const char *u : {"B", "kB", "MB", "GB", "TB"})
					{
						if(lineFields[2] == u)
						{
							return res;
						}

						res *= 1000;
					}
				}
			}
		}
	}

	return -1;
}

void cyclesToTime(uint64_t cycles, QString &time)
{
	static const std::vector<std::pair<const char*, uint64_t>> convTable =
	{
		{"d", 24 * 60 * 60 * 1'000'000'000ull},
		{"h", 60 * 60 * 1'000'000'000ull},
		{"m", 60 * 1'000'000'000ull},
		{"s", 1'000'000'000ull},
		{"ms", 1'000'000},
		{"us", 1'000},
		{"ns", 1}
	};

	cycles *= 8;
	bool first = true;

	for(const auto &e : convTable)
	{
		uint64_t step = cycles / e.second;

		if(step || (first && e.second == 1))
		{
			if(!first)
			{
				time += " + ";
			}

			first = false;

			time += QString::number(step);
			time += " ";
			time += e.first;
		}

		cycles %= e.second;
	}
}

void bytesToHumanReadable(uint64_t bytes, QString &res, bool dec)
{
	static const std::vector<const char*> convTableBin =
	{
		"B",   "KiB", "MiB",
		"GiB", "TiB", "PiB",
		"EiB", "ZiB", "YiB"
	};

	static const std::vector<const char*> convTableDec =
	{
		"B",  "KB", "MB",
		"GB", "TB", "PB",
		"EB", "ZB", "YB"
	};

	int idx = 0;
	float div = bytes;

	if(!dec)
	{
		if(bytes & ~0x3FF)
		{
			while(bytes & ~0xFFFFF)
			{
				idx += 1;
				bytes >>= 10;
			}

			idx++;
			div = bytes / 1024.0;
		}

		res = QString::number(div, 'f', 2);
		res += " ";
		res += convTableBin[idx];
	}
	else
	{
		if(bytes >= 1000)
		{
			while(bytes >= 1'000'000)
			{
				idx += 1;
				bytes /= 1000;
			}

			idx++;
			div = bytes / 1000.0;
		}

		res = QString::number(div, 'f', 2);
		res += " ";
		res += convTableDec[idx];
	}
}

void bitsToHumanReadable(uint64_t bits, QString &res, bool dec)
{
	bytesToHumanReadable(bits, res, dec);

	res.chop(1);
	res.push_back('b');
}
