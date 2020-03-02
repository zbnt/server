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

#include <FdtUtils.hpp>
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

void FrameDetector::announce(QByteArray &output) const
{
	if(!isReady()) return;

	appendAsBytes<uint8_t>(output, m_idx);
	appendAsBytes<uint8_t>(output, DEV_FRAME_DETECTOR);
	appendAsBytes<uint16_t>(output, 42);

	appendAsBytes<uint16_t>(output, PROP_PORTS);
	appendAsBytes<uint16_t>(output, 2);
	appendAsBytes<uint8_t>(output, m_portA);
	appendAsBytes<uint8_t>(output, m_portB);

	appendAsBytes<uint16_t>(output, PROP_FEATURE_BITS);
	appendAsBytes<uint16_t>(output, 4);
	appendAsBytes<uint32_t>(output, m_regs->features);

	appendAsBytes<uint16_t>(output, PROP_NUM_SCRIPTS);
	appendAsBytes<uint16_t>(output, 4);
	appendAsBytes<uint32_t>(output, m_regs->num_scripts);

	appendAsBytes<uint16_t>(output, PROP_MAX_SCRIPT_SIZE);
	appendAsBytes<uint16_t>(output, 4);
	appendAsBytes<uint32_t>(output, m_regs->max_script_size);

	appendAsBytes<uint16_t>(output, PROP_FIFO_SIZE);
	appendAsBytes<uint16_t>(output, 8);
	appendAsBytes<uint32_t>(output, m_regs->tx_fifo_size);
	appendAsBytes<uint32_t>(output, m_regs->extr_fifo_size);
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

	m_regs->config = CFG_LOG_ENABLE | CFG_ENABLE;
	m_regs->log_identifier = m_idx | MSG_ID_MEASUREMENT;
	m_scriptNames.fill("", 2*m_regs->num_scripts);

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

		case PROP_ENABLE_SCRIPT:
		{
			if(value.length() < 4) return false;

			m_regs->script_enable = readAsNumber<uint32_t>(value, 0);
			break;
		}

		case PROP_OVERFLOW_COUNT:
		{
			// read-only
			break;
		}

		case PROP_FRAME_SCRIPT:
		{
			if(value.length() < 4) return false;
			if(value.length() % 4) return false;

			uint32_t idx = readAsNumber<uint32_t>(value, 0);
			if(idx >= 2*m_regs->num_scripts) return false;

			uint32_t mask = 1 << idx;
			uint32_t enable = m_regs->script_enable;
			uint32_t *ptr = makePointer<uint32_t>(m_regs, m_regs->script_mem_offset + 4 * idx * m_regs->max_script_size);

			m_regs->script_enable = enable & ~mask;

			for(int i = 0, j = 4, k = m_regs->max_script_size; i < k; ++i, j += 4)
			{
				if(j < value.length())
				{
					ptr[i] = readAsNumber<uint32_t>(value, j);
				}
				else
				{
					ptr[i] = 0;
				}
			}

			m_regs->script_enable = enable;
			break;
		}

		case PROP_FRAME_SCRIPT_NAME:
		{
			if(value.length() < 4) return false;

			uint32_t idx = readAsNumber<uint32_t>(value, 0);
			if(idx >= 2*m_regs->num_scripts) return false;

			m_scriptNames[idx] = value.mid(4, -1);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool FrameDetector::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
{
	if(!isReady()) return false;

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

		case PROP_ENABLE_SCRIPT:
		{
			appendAsBytes(value, m_regs->script_enable);
			break;
		}

		case PROP_OVERFLOW_COUNT:
		{
			appendAsBytes(value, m_regs->overflow_count_a);
			appendAsBytes(value, m_regs->overflow_count_b);
			break;
		}

		case PROP_FRAME_SCRIPT:
		{
			if(params.length() < 4) return false;

			uint32_t idx = readAsNumber<uint32_t>(params, 0);
			if(idx >= 2*m_regs->num_scripts) return false;

			uint32_t *ptr = makePointer<uint32_t>(m_regs, m_regs->script_mem_offset + 4 * idx * m_regs->max_script_size);

			for(int i = 0, j = m_regs->max_script_size; i < j; ++i)
			{
				appendAsBytes(value, ptr[i]);
			}

			break;
		}

		case PROP_FRAME_SCRIPT_NAME:
		{
			if(params.length() < 4) return false;

			uint32_t idx = readAsNumber<uint32_t>(params, 0);
			if(idx >= 2*m_regs->num_scripts) return false;

			value.append(m_scriptNames[idx]);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}
