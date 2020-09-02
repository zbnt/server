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

#include <cores/SimpleTimer.hpp>

#include <QDebug>

#include <AbstractDevice.hpp>
#include <FdtUtils.hpp>

SimpleTimer::SimpleTimer(const QString &name, uint32_t id, void *regs)
	: AbstractCore(name, id), m_regs((volatile Registers*) regs)
{
	m_regs->max_time = 125'000'000;
}

SimpleTimer::~SimpleTimer()
{ }

AbstractCore *SimpleTimer::createCore(AbstractDevice *parent, const QString &name, uint32_t id,
                                      void *regs, const void *fdt, int offset)
{
	Q_UNUSED(parent);
	Q_UNUSED(fdt);
	Q_UNUSED(offset);

	return new SimpleTimer(name, id, regs);
}

void SimpleTimer::announce(QByteArray &output) const
{
	appendAsBytes<uint8_t>(output, m_id);
	appendAsBytes<uint8_t>(output, DEV_SIMPLE_TIMER);
	appendAsBytes<uint16_t>(output, 8);

	appendAsBytes<uint16_t>(output, PROP_CLOCK_FREQ);
	appendAsBytes<uint16_t>(output, 4);
	appendAsBytes<uint32_t>(output, 125'000'000);
}

DeviceType SimpleTimer::getType() const
{
	return DEV_SIMPLE_TIMER;
}

void SimpleTimer::setRunning(bool running)
{
	m_regs->config = (m_regs->config & ~CFG_ENABLE) | (running & CFG_ENABLE);
}

void SimpleTimer::setMaximumTime(uint64_t time)
{
	m_regs->max_time = time;
}

void SimpleTimer::setReset(bool reset)
{
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

bool SimpleTimer::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
{
	Q_UNUSED(params);

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
