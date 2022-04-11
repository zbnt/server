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

#include <FdtUtils.hpp>

bool fdtEnumerateDevices(const void *fdt, int offset, const std::function<bool(const QString&, int, int)> &callback)
{
	for(int node = fdt_first_subnode(fdt, offset); node >= 0; node = fdt_next_subnode(fdt, node))
	{
		int nameLen = 0;
		const char *name = fdt_get_name(fdt, node, &nameLen);

		if(!callback(QString::fromUtf8(QByteArray(name, nameLen)), node, offset))
		{
			return false;
		}

		fdtEnumerateDevices(fdt, node, callback);
	}

	return true;
}

bool fdtGetStringProp(const void *fdt, int offset, const char *name, QString &out)
{
	int len = 0;
	const void *data = fdt_getprop(fdt, offset, name, &len);

	if(!data)
	{
		return false;
	}

	out = QString::fromUtf8(QByteArray((const char*) data, len));
	return true;
}
