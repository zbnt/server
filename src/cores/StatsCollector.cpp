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

#include <cores/StatsCollector.hpp>

#include <QDebug>

#include <AbstractDevice.hpp>
#include <FdtUtils.hpp>

StatsCollector::StatsCollector(const QString &name, uint32_t id, void *regs, uint8_t port)
	: AbstractCore(name, id), m_regs((volatile Registers*) regs), m_port(port)
{
	m_regs->config = CFG_LOG_ENABLE | CFG_ENABLE;
	m_regs->sample_period = 12500000;
	m_regs->log_identifier = m_id | MSG_ID_MEASUREMENT;

	qInfo("[core] %s is connected to port %d", qUtf8Printable(name), port);
}

StatsCollector::~StatsCollector()
{ }

AbstractCore *StatsCollector::createCore(AbstractDevice *parent, const QString &name, uint32_t id,
                                         void *regs, const void *fdt, int offset)
{
	Q_UNUSED(parent);

	uint8_t port = 0;

	if(!fdtGetArrayProp(fdt, offset, "zbnt,ports", port))
	{
		qCritical("[core] E: Device tree lacks a valid value for zbnt,ports");
		return nullptr;
	}

	return new StatsCollector(name, id, regs, port);
}

void StatsCollector::announce(QByteArray &output) const
{
	appendAsBytes<uint8_t>(output, m_id);
	appendAsBytes<uint8_t>(output, DEV_STATS_COLLECTOR);
	appendAsBytes<uint16_t>(output, 5);

	appendAsBytes<uint16_t>(output, PROP_PORTS);
	appendAsBytes<uint16_t>(output, 1);
	appendAsBytes<uint8_t>(output, m_port);
}

DeviceType StatsCollector::getType() const
{
	return DEV_STATS_COLLECTOR;
}

uint64_t StatsCollector::getPorts() const
{
	return m_port;
}

void StatsCollector::setReset(bool reset)
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

bool StatsCollector::setProperty(PropertyID propID, const QByteArray &value)
{
	switch(propID)
	{
		case PROP_ENABLE:
		{
			if(value.length() < 1) return false;

			m_regs->config = (m_regs->config & ~CFG_ENABLE) | (readAsNumber<uint8_t>(value, 0) & CFG_ENABLE);
			break;
		}

		case PROP_ENABLE_LOG:
		{
			if(value.length() < 1) return false;

			if(readAsNumber<uint8_t>(value, 0))
			{
				m_regs->config |= CFG_LOG_ENABLE;
			}
			else
			{
				m_regs->config &= ~CFG_LOG_ENABLE;
			}

			break;
		}

		case PROP_SAMPLE_PERIOD:
		{
			if(value.length() < 4) return false;

			m_regs->sample_period = readAsNumber<uint32_t>(value, 0);
			break;
		}

		case PROP_OVERFLOW_COUNT:
		{
			// read-only
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StatsCollector::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
{
	Q_UNUSED(params);

	switch(propID)
	{
		case PROP_ENABLE:
		{
			appendAsBytes<uint8_t>(value, m_regs->config & CFG_ENABLE);
			break;
		}

		case PROP_ENABLE_LOG:
		{
			appendAsBytes<uint8_t>(value, !!(m_regs->config & CFG_LOG_ENABLE));
			break;
		}

		case PROP_SAMPLE_PERIOD:
		{
			appendAsBytes(value, m_regs->sample_period);
			break;
		}

		case PROP_OVERFLOW_COUNT:
		{
			appendAsBytes(value, m_regs->overflow_count);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}
