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

#include <dev/StatsCollector.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <FdtUtils.hpp>
#include <BitstreamManager.hpp>

StatsCollector::StatsCollector(const QByteArray &name, uint32_t index)
	: AbstractDevice(name, index), m_regs(nullptr), m_regsSize(0), m_port(0)
{ }

StatsCollector::~StatsCollector()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}
}

void StatsCollector::announce(QByteArray &output) const
{
	if(!isReady()) return;

	appendAsBytes<uint8_t>(output, m_idx);
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

bool StatsCollector::isReady() const
{
	return !!m_regs;
}

bool StatsCollector::loadDevice(const void *fdt, int offset)
{
	if(isReady() || !fdt) return false;

	quintptr base;

	if(!fdtGetArrayProp(fdt, offset, "reg", base, m_regsSize))
	{
		qCritical("[dev] E: Device %s doesn't have a valid reg property.", m_name.constData());
		return false;
	}

	if(!fdtGetArrayProp(fdt, offset, "zbnt,ports", m_port))
	{
		qCritical("[dev] E: Device %s doesn't have a valid zbnt,ports property.", m_name.constData());
		return false;
	}

	// Find UIO device

	auto it = g_uioMap.find(m_name);

	if(it == g_uioMap.end())
	{
		qCritical("[dev] E: No UIO device found for %s", m_name.constData());
		return false;
	}

	qDebug("[dev] D: Found %s in %s, connected to eth%u.", m_name.constData(), it->constData(), m_port);

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

	m_regs->config = CFG_LOG_ENABLE | CFG_ENABLE;
	m_regs->sample_period = 12500000;
	m_regs->log_identifier = m_idx | MSG_ID_MEASUREMENT;

	close(fd);
	return true;
}

void StatsCollector::setReset(bool reset)
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

bool StatsCollector::setProperty(PropertyID propID, const QByteArray &value)
{
	if(!isReady()) return false;

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
	if(!isReady()) return false;

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
