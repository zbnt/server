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

#include <dev/SimpleTimer.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <FdtUtils.hpp>
#include <BitstreamManager.hpp>

SimpleTimer::SimpleTimer(const QByteArray &name)
	: AbstractDevice(name, 0x80000000), m_regs(nullptr), m_regsSize(0)
{ }

SimpleTimer::~SimpleTimer()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}
}

DeviceType SimpleTimer::getType() const
{
	return DEV_SIMPLE_TIMER;
}

bool SimpleTimer::isReady() const
{
	return !!m_regs;
}

bool SimpleTimer::loadDevice(const void *fdt, int offset)
{
	if(isReady() || !fdt) return false;

	quintptr base;

	if(!fdtGetArrayProp(fdt, offset, "reg", base, m_regsSize))
	{
		qCritical("[dev] E: Device %s doesn't have a valid reg property.", m_name.constData());
		return false;
	}

	// Find UIO device

	auto it = g_uioMap.find(m_name);

	if(it == g_uioMap.end())
	{
		qCritical("[dev] E: No UIO device found for %s", m_name.constData());
		return false;
	}

	qDebug("[dev] D: Found %s in %s.", m_name.constData(), it->constData());

	// Open memory map

	QByteArray uioDevice = "/dev/" + *it;
	int fd = open(uioDevice.constData(), O_RDWR | O_SYNC);

	if(fd == -1)
	{
		qCritical("[dev] E: Failed to open %s", uioDevice.constData());
		return false;
	}

	m_regs = (volatile Registers*) mmap(NULL, m_regsSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(!m_regs || m_regs == MAP_FAILED)
	{
		m_regs = nullptr;
		qCritical("[dev] E: Failed to mmap %s", uioDevice.constData());
		return false;
	}

	m_regs->max_time = 125'000'000;

	close(fd);
	return true;
}

void SimpleTimer::setReset(bool reset)
{
	if(!isReady()) return;

	if(reset)
	{
		m_regs->config = CFG_RESET;
	}
	else
	{
		m_regs->config = 0;
	}
}

bool SimpleTimer::setProperty(PropertyID propID, const QByteArray &value)
{
	if(!isReady()) return false;

	switch(propID)
	{
		case PROP_ENABLE:
		{
			if(value.length() < 1) return false;

			uint8_t enable = readAsNumber<uint8_t>(value, 0);
			m_regs->config = (m_regs->config & ~CFG_ENABLE) | (enable & CFG_ENABLE);

			if(!enable)
			{
				m_regs->current_time = 0;
			}

			break;
		}

		case PROP_TIMER_MODE:
		{
			// RESERVED
			break;
		}

		case PROP_TIMER_TIME:
		{
			// read-only
			break;
		}

		case PROP_TIMER_LIMIT:
		{
			if(value.length() < 8) return false;

			m_regs->max_time = readAsNumber<uint64_t>(value, 0);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool SimpleTimer::getProperty(PropertyID propID, QByteArray &value)
{
	if(!isReady()) return false;

	switch(propID)
	{
		case PROP_ENABLE:
		{
			appendAsBytes<uint8_t>(value, m_regs->config & CFG_ENABLE);
			break;
		}

		case PROP_TIMER_MODE:
		{
			// RESERVED
			break;
		}

		case PROP_TIMER_TIME:
		{
			appendAsBytes(value, m_regs->current_time);
			break;
		}

		case PROP_TIMER_LIMIT:
		{
			appendAsBytes(value, m_regs->max_time);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

uint64_t SimpleTimer::getCurrentTime() const
{
	return m_regs->current_time;
}

uint64_t SimpleTimer::getMaximumTime() const
{
	return m_regs->max_time;
}
