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

#include <dev/FrameDetector.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <Utils.hpp>
#include <BitstreamManager.hpp>

FrameDetector::FrameDetector(const QByteArray &name, uint32_t index)
	: AbstractDevice(name, index), m_regs(nullptr), m_regsSize(0), m_portA(0), m_portB(0)
{ }

FrameDetector::~FrameDetector()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}
}

DeviceType FrameDetector::getType() const
{
	return DEV_FRAME_DETECTOR;
}

uint64_t FrameDetector::getPorts() const
{
	return (m_portB << 8) | m_portA;
}

bool FrameDetector::isReady() const
{
	return !!m_regs;
}

bool FrameDetector::loadDevice(const void *fdt, int offset)
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

	m_regs->log_identifier = m_idx | MSG_ID_MEASUREMENT;

	close(fd);
	return true;
}

void FrameDetector::setReset(bool reset)
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

bool FrameDetector::setProperty(PropertyID propID, const QByteArray &value)
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

		case PROP_ENABLE_CSUM_FIX:
		{
			if(value.length() < 1) return false;

			if(readAsNumber<uint8_t>(value, 0))
			{
				m_regs->config |= CFG_FIX_CSUM;
			}
			else
			{
				m_regs->config &= ~CFG_FIX_CSUM;
			}

			break;
		}

		case PROP_ENABLE_PATTERN:
		{
			if(value.length() < 4) return false;

			m_regs->match_enable = readAsNumber<uint32_t>(value, 0);
			break;
		}

		case PROP_OVERFLOW_COUNT:
		{
			// read-only
			break;
		}

		case PROP_FRAME_PATTERN:
		{
			if(value.length() < 2) return false;

			uint8_t dir = readAsNumber<uint8_t>(value, 0) & 1;
			uint8_t idx = readAsNumber<uint8_t>(value, 1) & 3;

			uint32_t mask = 1 << (16*dir + idx);
			uint32_t enable = m_regs->match_enable;
			uint8_t *ptr = makePointer<uint8_t>(m_regs, dir ? FD_PATTERN_B_DATA_OFFSET : FD_PATTERN_A_DATA_OFFSET);

			m_regs->match_enable = enable & ~mask;

			for(int i = idx, j = 2; i < FD_MEM_SIZE; i += 4, ++j)
			{
				if(j < value.length())
				{
					ptr[i] = value[j];
				}
				else
				{
					ptr[i] = 0;
				}
			}

			m_regs->match_enable = enable;
			break;
		}

		case PROP_FRAME_PATTERN_FLAGS:
		{
			if(value.length() < 2) return false;

			uint8_t dir = readAsNumber<uint8_t>(value, 0) & 1;
			uint8_t idx = readAsNumber<uint8_t>(value, 1) & 3;

			uint32_t mask = 1 << (16*dir + idx);
			uint32_t enable = m_regs->match_enable;
			uint8_t *ptr = makePointer<uint8_t>(m_regs, dir ? FD_PATTERN_B_FLAGS_OFFSET : FD_PATTERN_A_FLAGS_OFFSET);

			m_regs->match_enable = enable & ~mask;

			for(int i = idx, j = 2; i < FD_MEM_SIZE; i += 4, ++j)
			{
				if(j < value.length())
				{
					ptr[i] = value[j];
				}
				else
				{
					ptr[i] = 0;
				}
			}

			m_regs->match_enable = enable;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool FrameDetector::getProperty(PropertyID propID, QByteArray &value)
{
	if(!isReady()) return false;

	switch(propID)
	{
		case PROP_ENABLE:
		{
			appendAsBytes<uint8_t>(value, m_regs->config & CFG_ENABLE);
			break;
		}

		case PROP_ENABLE_CSUM_FIX:
		{
			appendAsBytes<uint8_t>(value, !!(m_regs->config & CFG_FIX_CSUM));
			break;
		}

		case PROP_ENABLE_PATTERN:
		{
			appendAsBytes(value, m_regs->match_enable);
			break;
		}

		case PROP_OVERFLOW_COUNT:
		{
			appendAsBytes(value, m_regs->overflow_count_a);
			appendAsBytes(value, m_regs->overflow_count_b);
			break;
		}

		case PROP_FRAME_PATTERN:
		{
			uint8_t *ptrA = makePointer<uint8_t>(m_regs, FD_PATTERN_A_DATA_OFFSET);
			uint8_t *ptrB = makePointer<uint8_t>(m_regs, FD_PATTERN_B_DATA_OFFSET);

			appendAsBytes<uint32_t>(value, FD_MEM_SIZE / FD_NUM_PATTERNS);

			for(uint8_t *ptr : {ptrA, ptrB})
			{
				for(int idx = 0; idx < FD_NUM_PATTERNS; ++idx)
				{
					for(int j = idx; j < FD_MEM_SIZE; j += 4)
					{
						value.append(ptr[j]);
					}
				}
			}

			break;
		}

		case PROP_FRAME_PATTERN_FLAGS:
		{
			uint8_t *ptrA = makePointer<uint8_t>(m_regs, FD_PATTERN_A_FLAGS_OFFSET);
			uint8_t *ptrB = makePointer<uint8_t>(m_regs, FD_PATTERN_B_FLAGS_OFFSET);

			appendAsBytes<uint32_t>(value, FD_MEM_SIZE / FD_NUM_PATTERNS);

			for(uint8_t *ptr : {ptrA, ptrB})
			{
				for(int idx = 0; idx < FD_NUM_PATTERNS; ++idx)
				{
					for(int j = idx; j < FD_MEM_SIZE; j += 4)
					{
						value.append(ptr[j]);
					}
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
