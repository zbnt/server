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

#include <cores/TrafficGenerator.hpp>

#include <QDebug>

#include <AbstractDevice.hpp>
#include <FdtUtils.hpp>

TrafficGenerator::TrafficGenerator(const QString &name, uint32_t id, void *regs, uint8_t port)
	: AbstractCore(name, id), m_regs((volatile Registers*) regs), m_port(port)
{
	m_regs->fsize = 60;
	m_regs->fdelay = 12;
	m_regs->burst_time_on = 100;
	m_regs->burst_time_off = 100;

	m_templateName = "";
	m_templateSize = 0;

	qInfo("[core] %s is connected to port %d", qUtf8Printable(name), port);
}

TrafficGenerator::~TrafficGenerator()
{ }

AbstractCore *TrafficGenerator::createCore(AbstractDevice *parent, const QString &name, uint32_t id,
                                           void *regs, const void *fdt, int offset)
{
	Q_UNUSED(parent);

	uint8_t port = 0;

	if(!fdtGetArrayProp(fdt, offset, "zbnt,ports", port))
	{
		qCritical("[core] E: Device tree lacks a valid value for zbnt,ports");
		return nullptr;
	}

	return new TrafficGenerator(name, id, regs, port);
}

void TrafficGenerator::announce(QByteArray &output) const
{
	appendAsBytes<uint8_t>(output, m_id);
	appendAsBytes<uint8_t>(output, DEV_TRAFFIC_GENERATOR);
	appendAsBytes<uint16_t>(output, 13);

	appendAsBytes<uint16_t>(output, PROP_PORTS);
	appendAsBytes<uint16_t>(output, 1);
	appendAsBytes<uint8_t>(output, m_port);

	appendAsBytes<uint16_t>(output, PROP_MAX_TEMPLATE_SIZE);
	appendAsBytes<uint16_t>(output, 4);
	appendAsBytes<uint32_t>(output, TGEN_MEM_SIZE);
}

DeviceType TrafficGenerator::getType() const
{
	return DEV_TRAFFIC_GENERATOR;
}

uint64_t TrafficGenerator::getPorts() const
{
	return m_port;
}

void TrafficGenerator::setReset(bool reset)
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

bool TrafficGenerator::setProperty(PropertyID propID, const QByteArray &value)
{
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

		case PROP_PRNG_SEED:
		{
			if(value.length() < 1) return false;

			m_regs->prng_seed_val = readAsNumber<uint8_t>(value, 0);
			m_regs->config |= CFG_SEED_REQ;
			break;
		}

		case PROP_FRAME_TEMPLATE:
		{
			// Extract name

			int nameLen = value.indexOf('\0');

			if(nameLen <= 0) return false;

			auto name = value.left(nameLen);

			// Get and check length of the actual data and source selectors

			int dataLen = value.length() - nameLen - 1;
			int templateLen = dataLen / 2;

			if(dataLen == 0 || (dataLen % 2) != 0) return false;

			// Disable transmission while we update the template

			uint32_t config = m_regs->config;

			m_regs->config = config & ~CFG_ENABLE;

			// Copy new values

			const char *srcA = value.constData() + nameLen + 1;
			const char *srcB = srcA + templateLen;

			uint8_t *dstA = makePointer<uint8_t>(m_regs, TGEN_MEM_TEMPLATE_OFFSET);
			uint8_t *dstB = makePointer<uint8_t>(m_regs, TGEN_MEM_SOURCE_OFFSET);

			for(int i = 0; i < TGEN_MEM_SIZE; ++i)
			{
				if(i < templateLen)
				{
					dstA[i] = srcA[i];
					dstB[i] = srcB[i];
				}
				else
				{
					dstA[i] = 0x00;
					dstB[i] = 0x01;
				}
			}

			m_templateName = name;
			m_templateSize = templateLen;

			// Resume operation

			m_regs->config = config;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool TrafficGenerator::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
{
	Q_UNUSED(params);

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

		case PROP_PRNG_SEED:
		{
			appendAsBytes(value, m_regs->prng_seed_val);
			break;
		}

		case PROP_FRAME_TEMPLATE:
		{
			// Name

			value.append(m_templateName);
			value.push_back('\0');

			// Data bytes

			char *srcA = makePointer<char>(m_regs, TGEN_MEM_TEMPLATE_OFFSET);

			value.append(srcA, m_templateSize);

			// Source selectors

			char *srcB = makePointer<char>(m_regs, TGEN_MEM_SOURCE_OFFSET);

			value.append(srcB, m_templateSize);
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}
