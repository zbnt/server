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

#include <DmaBuffer.hpp>
#include <cores/AxiDma.hpp>
#include <cores/SimpleTimer.hpp>

using CoreList = QVector<AbstractCore*>;
using BitstreamList = QVector<QString>;
using MmapList = QVector<QPair<void*, size_t>>;

class AbstractCore;

class AbstractDevice
{
public:
	AbstractDevice();
	virtual ~AbstractDevice();

	virtual bool waitForInterrupt(int timeout) = 0;
	virtual void clearInterrupts(uint16_t irq) = 0;

	virtual bool loadBitstream(const QString &name) = 0;
	virtual const QString &activeBitstream() const = 0;
	virtual const BitstreamList &bitstreamList() const = 0;

	SimpleTimer *timer() const;
	AxiDma *dmaEngine() const;
	const DmaBuffer *dmaBuffer() const;
	const CoreList &coreList() const;

protected:
	AxiDma *m_dmaEngine = nullptr;
	DmaBuffer *m_dmaBuffer = nullptr;
	SimpleTimer *m_timer = nullptr;

	CoreList m_coreList;
};
