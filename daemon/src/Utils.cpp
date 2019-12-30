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

#include <QFile>
#include <QVector>
#include <QTextStream>

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

bool fdtEnumerateDevices(const void *fdt, int offset, const std::function<bool(const QByteArray&, int)> &callback)
{
	for(int node = fdt_first_subnode(fdt, offset); node >= 0; node = fdt_next_subnode(fdt, node))
	{
		int nameLen = 0;
		const char *name = fdt_get_name(fdt, node, &nameLen);

		if(!callback(QByteArray(name, nameLen), node))
		{
			return false;
		}

		fdtEnumerateDevices(fdt, node, callback);
	}

	return true;
}

bool fdtGetStringProp(const void *fdt, int offset, const char *name, QByteArray &out)
{
	int len = 0;
	const void *data = fdt_getprop(fdt, offset, name, &len);

	if(!data)
	{
		return false;
	}

	out = QByteArray((const char*) data, len);
	return true;
}
