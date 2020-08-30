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

#include <dev/LatencyMeasurer.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <FdtUtils.hpp>
#include <BitstreamManager.hpp>

LatencyMeasurer::LatencyMeasurer(const QByteArray &name, uint32_t index)
	: AbstractDevice(name, index), m_regs(nullptr), m_regsSize(0), m_portA(0), m_portB(0)
{ }

LatencyMeasurer::~LatencyMeasurer()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}
}

void LatencyMeasurer::announce(QByteArray &output) const
{
	if(!isReady()) return;

	appendAsBytes<uint8_t>(output, m_idx);
	appendAsBytes<uint8_t>(output, DEV_LATENCY_MEASURER);
	appendAsBytes<uint16_t>(output, 6);

	appendAsBytes<uint16_t>(output, PROP_PORTS);
	appendAsBytes<uint16_t>(output, 2);
	appendAsBytes<uint8_t>(output, m_portA);
	appendAsBytes<uint8_t>(output, m_portB);
}

DeviceType LatencyMeasurer::getType() const
{
	return DEV_LATENCY_MEASURER;
}

uint64_t LatencyMeasurer::getPorts() const
{
	return (m_portB << 8) | m_portA;
}

bool LatencyMeasurer::isReady() const
{
	return !!m_regs;
}

bool LatencyMeasurer::loadDevice(const void *fdt, int offset)
{
	if(!fdt || m_regs) return false;

	quintptr base;

	if(!fdtGetArrayProp(fdt, offset, "reg", base, m_regsSize))
	{
		qCritical("[dev] E: Device %s doesn't have a valid reg property.", m_name.constData());
		return false;
	}

	if(!fdtGetArrayProp(fdt, offset, "zbnt,ports", m_portA, m_portB))
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

	qDebug("[dev] D: Found %s in %s, connected to eth%u and eth%u.", m_name.constData(), it->constData(), m_portA, m_portB);

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

	m_regs->config = 0;
	m_regs->padding = 18;
	m_regs->timeout = 125000000;
	m_regs->delay = 12500000;
	m_regs->log_identifier = m_idx | MSG_ID_MEASUREMENT;

	close(fd);
	return true;
}

void LatencyMeasurer::setReset(bool reset)
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

bool LatencyMeasurer::setProperty(PropertyID propID, const QByteArray &value)
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

		case PROP_ENABLE_BROADCAST:
		{
			if(value.length() < 1) return false;

			if(readAsNumber<uint8_t>(value, 0))
			{
				m_regs->config |= CFG_BROADCAST;
			}
			else
			{
				m_regs->config &= ~CFG_BROADCAST;
			}

			break;
		}

		case PROP_MAC_ADDR:
		{
			if(value.length() < 7) return false;

			uint8_t idx = value[0] & 1;
			uint16_t cfg = m_regs->config;
			volatile uint8_t *ptr = idx ? m_regs->mac_addr_b : m_regs->mac_addr_a;

			m_regs->config = cfg & ~CFG_ENABLE;

			for(int i = 0, j = 1; i < 6; ++i, ++j)
			{
				ptr[i] = value[j];
			}

			m_regs->config = cfg;
			break;
		}

		case PROP_IP_ADDR:
		{
			if(value.length() < 5) return false;

			uint8_t idx = value[0] & 1;

			if(!idx)
			{
				m_regs->ip_addr_a = readAsNumber<uint32_t>(value, 1);
			}
			else
			{
				m_regs->ip_addr_b = readAsNumber<uint32_t>(value, 1);
			}

			break;
		}

		case PROP_FRAME_PADDING:
		{
			if(value.length() < 2) return false;

			m_regs->padding = readAsNumber<uint16_t>(value, 0);
			break;
		}

		case PROP_FRAME_GAP:
		{
			if(value.length() < 4) return false;

			m_regs->delay = readAsNumber<uint32_t>(value, 0);
			break;
		}

		case PROP_TIMEOUT:
		{
			if(value.length() < 4) return false;

			m_regs->timeout = readAsNumber<uint32_t>(value, 0);
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

bool LatencyMeasurer::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
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

		case PROP_ENABLE_BROADCAST:
		{
			appendAsBytes<uint8_t>(value, !!(m_regs->config & CFG_BROADCAST));
			break;
		}

		case PROP_MAC_ADDR:
		{
			value.append((char*) m_regs->mac_addr_a, 6);
			value.append((char*) m_regs->mac_addr_b, 6);
			break;
		}

		case PROP_IP_ADDR:
		{
			appendAsBytes(value, m_regs->ip_addr_a);
			appendAsBytes(value, m_regs->ip_addr_b);
			break;
		}

		case PROP_FRAME_PADDING:
		{
			appendAsBytes<uint16_t>(value, m_regs->padding);
			break;
		}

		case PROP_FRAME_GAP:
		{
			appendAsBytes(value, m_regs->delay);
			break;
		}

		case PROP_TIMEOUT:
		{
			appendAsBytes(value, m_regs->timeout);
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