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

#include <AbstractDevice.hpp>

#include <unistd.h>
#include <poll.h>

#include <QFile>

AbstractDevice::AbstractDevice()
{ }

AbstractDevice::~AbstractDevice()
{ }

bool AbstractDevice::waitForInterrupt(int timeout)
{
	if(m_irqfd == -1) return false;
	if(!m_dmaEngine) return false;
	if(!m_dmaBuffer) return false;

	pollfd pfd;
	pfd.fd = m_irqfd;
	pfd.events = POLLIN;

	if(poll(&pfd, 1, timeout) >= 1)
	{
		read(m_irqfd, &m_irq, sizeof(uint32_t));
		return true;
	}

	return false;
}

void AbstractDevice::clearInterrupts(uint16_t irq)
{
	if(m_irqfd == -1) return;
	if(!m_dmaEngine) return;

	m_dmaEngine->clearInterrupts(irq);

	write(m_irqfd, &m_irq, sizeof(uint32_t));
}

SimpleTimer *AbstractDevice::timer() const
{
	return m_timer;
}

AxiDma *AbstractDevice::dmaEngine() const
{
	return m_dmaEngine;
}

const DmaBuffer *AbstractDevice::dmaBuffer() const
{
	return m_dmaBuffer;
}

const CoreList &AbstractDevice::coreList() const
{
	return m_coreList;
}

QByteArray AbstractDevice::dumpFile(const QString &path, bool *ok)
{
	QFile dumpedFile(path);

	if(!dumpedFile.open(QIODevice::ReadOnly))
	{
		if(ok)
		{
			*ok = false;
		}

		return "";
	}

	if(ok)
	{
		*ok = true;
	}

	return dumpedFile.readAll();
}
