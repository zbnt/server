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
#include <cores/PrController.hpp>

class PciDevice : public AbstractDevice
{
	struct RegionHeader
	{
		char magic[5];
		char type[3];

		uint32_t version_base;
		char version_prerel[16];
		char version_commit[16];
		uint16_t version_dirty;

		uint16_t dtb_size;
	};

public:
	PciDevice(const QString &device);
	~PciDevice();

	bool waitForInterrupt();
	void clearInterrupts();

	bool loadBitstream(const QString &name);
	const QString &activeBitstream() const;
	const BitstreamList &bitstreamList() const;

private:
	int m_container = -1;
	int m_group = -1;
	int m_device = -1;
	int m_irqfd = -1;

	off_t m_confRegion = 0;
	MmapList m_memMaps;
	QString m_boardName = "<unknown>";

	PrController *m_prCtl = nullptr;
	BitstreamList m_bitstreamList;
};
