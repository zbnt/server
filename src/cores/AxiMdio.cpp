/*
	zbnt/server
	Copyright (C) 2020 Oscar R.

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

#include <cores/AxiMdio.hpp>

#include <QDebug>

#include <AbstractDevice.hpp>
#include <FdtUtils.hpp>

AxiMdio::AxiMdio(const QString &name, uint32_t id, void *regs, const QList<uint8_t> &ports, const QList<uint8_t> &phys)
	: AbstractCore(name, id), m_regs((volatile Registers*) regs), m_ports(ports), m_phys(phys)
{
	qInfo("[core] %s is connected to:", qUtf8Printable(name));

	for(int i = 0; i < ports.size(); ++i)
	{
		qInfo("           - Port %d, phy address %02X", ports[i], phys[i]);
	}
}

AxiMdio::~AxiMdio()
{ }

AbstractCore *AxiMdio::createCore(AbstractDevice *parent, const QString &name, uint32_t id,
                                  void *regs, const void *fdt, int offset)
{
	Q_UNUSED(parent);

	QList<uint8_t> ports, phys;

	int length = 0;
	const uint8_t *portList = (const uint8_t*) fdt_getprop(fdt, offset, "zbnt,ports", &length);

	if(portList)
	{
		while(length >= 4)
		{
			uint32_t portID;

			if(fdtArrayToVars(portList, length, portID) && portID <= 255)
			{
				ports.append(portID);
			}

			length -= 4;
			portList += 4;
		}
	}
	else
	{
		qCritical("[core] E: Device tree lacks a valid value for zbnt,ports");
		return nullptr;
	}

	const uint8_t *phyList = (const uint8_t*) fdt_getprop(fdt, offset, "zbnt,phy-addr", &length);

	if(phyList)
	{
		while(length >= 4)
		{
			uint32_t phyAddr;

			if(fdtArrayToVars(phyList, length, phyAddr) && phyAddr <= 31)
			{
				phys.append(phyAddr);
			}

			length -= 4;
			phyList += 4;
		}
	}
	else
	{
		qCritical("[core] E: Device tree lacks a valid value for zbnt,phy-addr");
		return nullptr;
	}

	if(ports.size() != phys.size())
	{
		qCritical("[core] E: Number of ports and PHYs in device doesn't match");
		return nullptr;
	}

	// Initialize MDIO registers

	AxiMdio *core = new AxiMdio(name, id, regs, ports, phys);

	const uint8_t *initSeq = (const uint8_t*) fdt_getprop(fdt, offset, "zbnt,init-seq", &length);

	if(initSeq)
	{
		while(length >= 12)
		{
			uint32_t phyAddr, regAddr, value;

			if(fdtArrayToVars(initSeq, length, phyAddr, regAddr, value))
			{
				if(phyAddr > 31) continue;

				if(regAddr > 31)
				{
					core->writePhyIndirect(phyAddr, regAddr, value);
				}
				else
				{
					core->writePhy(phyAddr, regAddr, value);
				}
			}

			length -= 12;
			initSeq += 12;
		}
	}

	return core;
}

void AxiMdio::announce(QByteArray &output) const
{
	appendAsBytes<uint8_t>(output, m_id);
	appendAsBytes<uint8_t>(output, DEV_AXI_MDIO);
	appendAsBytes<uint16_t>(output, 8 + m_ports.size() + m_phys.size());

	appendAsBytes<uint16_t>(output, PROP_PORTS);
	appendAsBytes<uint16_t>(output, m_ports.size());

	for(uint8_t p : m_ports)
	{
		appendAsBytes<uint8_t>(output, p);
	}

	appendAsBytes<uint16_t>(output, PROP_PHY_ADDR);
	appendAsBytes<uint16_t>(output, m_phys.size());

	for(uint8_t p : m_phys)
	{
		appendAsBytes<uint8_t>(output, p);
	}
}

DeviceType AxiMdio::getType() const
{
	return DEV_AXI_MDIO;
}

uint32_t AxiMdio::readPhy(uint32_t phyAddr, uint32_t regAddr)
{
	m_regs->addr = OP_READ | (phyAddr << 5) | regAddr;
	m_regs->ctl = CTL_ENABLE | CTL_START;

	while(m_regs->ctl & CTL_START);

	return m_regs->rd_data;
}

uint32_t AxiMdio::readPhyIndirect(uint32_t phyAddr, uint32_t regAddr)
{
	writePhy(phyAddr, 0x0D, 0x001F);
	writePhy(phyAddr, 0x0E, regAddr);
	writePhy(phyAddr, 0x0D, 0x401F);
	return readPhy(phyAddr, 0x0E);
}

void AxiMdio::writePhy(uint32_t phyAddr, uint32_t regAddr, uint32_t value)
{
	m_regs->addr = (phyAddr << 5) | regAddr;
	m_regs->wr_data = value;
	m_regs->ctl = CTL_ENABLE | CTL_START;

	while(m_regs->ctl & CTL_START);
}

void AxiMdio::writePhyIndirect(uint32_t phyAddr, uint32_t regAddr, uint32_t value)
{
	writePhy(phyAddr, 0x0D, 0x001F);
	writePhy(phyAddr, 0x0E, regAddr);
	writePhy(phyAddr, 0x0D, 0x401F);
	writePhy(phyAddr, 0x0E, value);
}

void AxiMdio::setReset(bool reset)
{
	Q_UNUSED(reset);
}

bool AxiMdio::setProperty(PropertyID propID, const QByteArray &value)
{
	Q_UNUSED(propID);
	Q_UNUSED(value);
	return false;
}

bool AxiMdio::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
{
	Q_UNUSED(propID);
	Q_UNUSED(params);
	Q_UNUSED(value);
	return false;
}
