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

#include <cores/PrController.hpp>

#include <unistd.h>

#include <QDebug>

#include <AbstractDevice.hpp>
#include <FdtUtils.hpp>

PrController::PrController(const QString &name, uint32_t id, void *regs, const QVector<QString> &bitstreams)
	: AbstractCore(name, id), m_regs((volatile Registers*) regs), m_bitstreamNames(bitstreams)
{
	qInfo("[core] %s has %d bitstreams available:", qUtf8Printable(name), bitstreams.size());

	for(const QString &n : bitstreams)
	{
		qInfo("           - %s", qUtf8Printable(n));
	}
}

PrController::~PrController()
{ }

AbstractCore *PrController::createCore(AbstractDevice *parent, const QString &name, uint32_t id,
                                       void *regs, const void *fdt, int offset)
{
	Q_UNUSED(parent);

	int bsCount = fdt_stringlist_count(fdt, offset, "zbnt,bitstreams");

	if(bsCount <= 0)
	{
		qCritical("[core] E: Device tree lacks a valid value for zbnt,bitstreams");
		return nullptr;
	}

	BitstreamList bitstreams;
	int bsNameLen = 0;

	for(int i = 0; i < bsCount; ++i)
	{
		const char *bsName = fdt_stringlist_get(fdt, offset, "zbnt,bitstreams", i, &bsNameLen);

		if(bsName && bsNameLen > 0)
		{
			bitstreams.append(QString::fromUtf8(bsName, bsNameLen));
		}
	}

	if(!bitstreams.size())
	{
		qCritical("[core] E: No valid partial bitstreams found in device tree");
		return nullptr;
	}

	return new PrController(name, id, regs, bitstreams);
}

void PrController::announce(QByteArray &output) const
{
	Q_UNUSED(output);
}

DeviceType PrController::getType() const
{
	return DEV_PR_CONTROLLER;
}

bool PrController::loadBitstream(const QString &name)
{
	int idx = m_bitstreamNames.indexOf(name);

	if(idx < 0)
	{
		return false;
	}

	if(m_regs->status & ST_SHUTDOWN)
	{
		m_regs->status = CMD_RESTART;
		usleep(1000);
	}

	m_regs->trigger = idx;

	do
	{
		usleep(1000);
	}
	while(m_regs->status & TRIGGER_PENDING);

	return ((m_regs->status & 0xFF) == ST_ACTIVE_OKAY);
}

const QString &PrController::activeBitstream() const
{
	uint32_t offset = (m_regs->status >> 8) & 0xFFFF;

	if(offset > m_bitstreamNames.size())
	{
		offset = 0;
	}

	return m_bitstreamNames[offset];
}

const BitstreamList PrController::bitstreamList() const
{
	return m_bitstreamNames;
}

void PrController::setReset(bool reset)
{
	Q_UNUSED(reset);
}

bool PrController::setProperty(PropertyID propID, const QByteArray &value)
{
	Q_UNUSED(propID);
	Q_UNUSED(value);
	return false;
}

bool PrController::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
{
	Q_UNUSED(propID);
	Q_UNUSED(params);
	Q_UNUSED(value);
	return false;
}
