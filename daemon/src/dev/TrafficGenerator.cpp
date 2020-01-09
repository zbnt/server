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

#include <dev/TrafficGenerator.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <FdtUtils.hpp>
#include <BitstreamManager.hpp>

TrafficGenerator::TrafficGenerator(const QByteArray &name, uint32_t index)
	: AbstractDevice(name, index), m_regs(nullptr), m_regsSize(0), m_port(0)
{ }

TrafficGenerator::~TrafficGenerator()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}
}

DeviceType TrafficGenerator::getType() const
{
	return DEV_TRAFFIC_GENERATOR;
}

uint64_t TrafficGenerator::getPorts() const
{
	return m_port;
}

bool TrafficGenerator::isReady() const
{
	return !!m_regs;
}

bool TrafficGenerator::loadDevice(const void *fdt, int offset)
{
	if(!fdt || m_regs) return false;

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

	m_regs->fsize = 60;
	m_regs->fdelay = 12;
	m_regs->burst_time_on = 100;
	m_regs->burst_time_off = 100;

	close(fd);
	return true;
}

void TrafficGenerator::setReset(bool reset)
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

bool TrafficGenerator::setProperty(PropertyID propID, const QByteArray &value)
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

		case PROP_ENABLE_BURST:
		{
			if(value.length() < 1) return false;

			if(readAsNumber<uint8_t>(value, 0))
			{
				m_regs->config |= CFG_BURST;
			}
			else
			{
				m_regs->config &= ~CFG_BURST;
			}

			break;
		}

		case PROP_FRAME_SIZE:
		{
			if(value.length() < 2) return false;

			m_regs->fsize = readAsNumber<uint16_t>(value, 0);
			break;
		}

		case PROP_FRAME_GAP:
		{
			if(value.length() < 4) return false;

			m_regs->fdelay = readAsNumber<uint32_t>(value, 0);
			break;
		}

		case PROP_BURST_TIME_ON:
		{
			if(value.length() < 2) return false;

			m_regs->burst_time_on = readAsNumber<uint16_t>(value, 0);
			break;
		}

		case PROP_BURST_TIME_OFF:
		{
			if(value.length() < 2) return false;

			m_regs->burst_time_off = readAsNumber<uint16_t>(value, 0);
			break;
		}

		case PROP_LFSR_SEED:
		{
			if(value.length() < 1) return false;

			m_regs->lfsr_seed_val = readAsNumber<uint8_t>(value, 0);
			m_regs->config |= CFG_SEED_REQ;
			break;
		}

		case PROP_FRAME_TEMPLATE:
		{
			uint8_t *ptr = makePointer<uint8_t>(m_regs, TGEN_MEM_FRAME_OFFSET);

			for(int i = 0; i < TGEN_MEM_SIZE; ++i)
			{
				if(i < value.length())
				{
					ptr[i] = value[i];
				}
				else
				{
					ptr[i] = 0;
				}
			}

			break;
		}

		case PROP_FRAME_PATTERN:
		{
			uint8_t *ptr = makePointer<uint8_t>(m_regs, TGEN_MEM_PATTERN_OFFSET);

			for(int i = 0; i < TGEN_MEM_SIZE/8; ++i)
			{
				if(i < value.length())
				{
					ptr[i] = value[i];
				}
				else
				{
					ptr[i] = 0xFF;
				}
			}

			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool TrafficGenerator::getProperty(PropertyID propID, QByteArray &value)
{
	if(!isReady()) return false;

	switch(propID)
	{
		case PROP_ENABLE:
		{
			appendAsBytes<uint8_t>(value, m_regs->config & CFG_ENABLE);
			break;
		}

		case PROP_ENABLE_BURST:
		{
			appendAsBytes<uint8_t>(value, !!(m_regs->config & CFG_BURST));
			break;
		}

		case PROP_FRAME_SIZE:
		{
			appendAsBytes<uint16_t>(value, m_regs->fsize);
			break;
		}

		case PROP_FRAME_GAP:
		{
			appendAsBytes(value, m_regs->fdelay);
			break;
		}

		case PROP_BURST_TIME_ON:
		{
			appendAsBytes(value, m_regs->burst_time_on);
			break;
		}

		case PROP_BURST_TIME_OFF:
		{
			appendAsBytes(value, m_regs->burst_time_off);
			break;
		}

		case PROP_LFSR_SEED:
		{
			appendAsBytes(value, m_regs->lfsr_seed_val);
			break;
		}

		case PROP_FRAME_TEMPLATE:
		{
			char *ptr = makePointer<char>(m_regs, TGEN_MEM_FRAME_OFFSET);

			value.append(ptr, TGEN_MEM_SIZE);
			break;
		}

		case PROP_FRAME_PATTERN:
		{
			char *ptr = makePointer<char>(m_regs, TGEN_MEM_PATTERN_OFFSET);

			value.append(ptr, TGEN_MEM_SIZE/8);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}
