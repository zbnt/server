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

#include <cores/FrameDetector.hpp>

#include <QDebug>

#include <AbstractDevice.hpp>
#include <FdtUtils.hpp>

FrameDetector::FrameDetector(const QString &name, uint32_t id, void *regs, uint8_t portA, uint8_t portB)
	: AbstractCore(name, id), m_regs((volatile Registers*) regs), m_portA(portA), m_portB(portB)
{
	m_regs->config = CFG_LOG_ENABLE | CFG_ENABLE;
	m_regs->log_identifier = m_id | MSG_ID_MEASUREMENT;

	m_scriptNames.fill("", 2 * m_regs->num_scripts);

	qInfo("[core] %s is connected to ports %d and %d", qUtf8Printable(name), portA, portB);
}

FrameDetector::~FrameDetector()
{ }

AbstractCore *FrameDetector::createCore(AbstractDevice *parent, const QString &name, uint32_t id,
                                        void *regs, const void *fdt, int offset)
{
	Q_UNUSED(parent);

	uint8_t portA = 0;
	uint8_t portB = 0;

	if(!fdtGetArrayProp(fdt, offset, "zbnt,ports", portA, portB))
	{
		qCritical("[core] E: Device tree lacks a valid value for zbnt,ports");
		return nullptr;
	}

	return new FrameDetector(name, id, regs, portA, portB);
}

void FrameDetector::announce(QByteArray &output) const
{
	appendAsBytes<uint8_t>(output, m_id);
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

void FrameDetector::setReset(bool reset)
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

bool FrameDetector::setProperty(PropertyID propID, const QByteArray &value)
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
