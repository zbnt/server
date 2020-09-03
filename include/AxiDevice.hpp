/*
	zbnt/server
	Copyright (C) 2020 Oscar R.

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
#include <QHash>

#include <AbstractDevice.hpp>

using MmapList = QVector<QPair<void*, size_t>>;

class AxiDevice : public AbstractDevice
{
public:
	AxiDevice();
	~AxiDevice();

	bool loadBitstream(const QString &name);
	const QString &activeBitstream() const;
	const BitstreamList &bitstreamList() const;

private:
	bool loadDeviceTree(const QString &name, const QByteArray &contents);

private:
	QHash<QString, QByteArray> m_uioMap;
	MmapList m_mmapList;

	QString m_activeBitstream;
	BitstreamList m_bitstreamList;
};
