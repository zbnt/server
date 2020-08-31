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

#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <FdtUtils.hpp>
#include <BitstreamManager.hpp>

AxiMdio::AxiMdio(const QByteArray &name, uint32_t index)
	: AbstractCore(name, index), m_regs(nullptr), m_regsSize(0)
{ }

AxiMdio::~AxiMdio()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}
}

void AxiMdio::announce(QByteArray &output) const
{
	if(!isReady()) return;

	appendAsBytes<uint8_t>(output, m_idx);
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

bool AxiMdio::isReady() const
{
	return m_regs;
}

bool AxiMdio::loadDevice(const void *fdt, int offset)
{
	if(!fdt || m_regs) return false;

	quintptr base;

	if(!fdtGetArrayProp(fdt, offset, "reg", base, m_regsSize))
	{
		qCritical("[dev] E: Device %s doesn't have a valid reg property.", m_name.constData());
		return false;
	}

	int length = 0;
	const uint8_t *portList = (const uint8_t*) fdt_getprop(fdt, offset, "zbnt,ports", &length);

	if(portList)
	{
		while(length >= 4)
		{
			uint32_t portID;

			if(fdtArrayToVars(portList, length, portID) && portID <= 255)
			{
				m_ports.append(portID);
			}

			length -= 4;
			portList += 4;
		}
	}
	else
	{
		qCritical("[dev] E: Device %s doesn't have a valid zbnt,ports property.", m_name.constData());
		return false;
	}

	const uint8_t *phyList = (const uint8_t*) fdt_getprop(fdt, offset, "zbnt,phy-addr", &length);

	if(phyList)
	{
		while(length >= 4)
		{
			uint32_t phyAddr;

			if(fdtArrayToVars(phyList, length, phyAddr) && phyAddr <= 31)
			{
				m_phys.append(phyAddr);
			}

			length -= 4;
			phyList += 4;
		}
	}
	else
	{
		qCritical("[dev] E: Device %s doesn't have a valid zbnt,phy-addr property.", m_name.constData());
		return false;
	}

	if(m_ports.size() != m_phys.size())
	{
		qCritical("[dev] E: Number of ports and PHYs in device %s doesn't match.", m_name.constData());
		return false;
	}

	// Find UIO device

	auto it = g_uioMap.find(m_name);

	if(it == g_uioMap.end())
	{
		qCritical("[dev] E: No UIO device found for %s", m_name.constData());
		return false;
	}

	qDebug("[dev] D: Found %s in %s.", m_name.constData(), it->constData());

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

	// Initialize MDIO registers

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
					writePhyIndirect(phyAddr, regAddr, value);
				}
				else
				{
					writePhy(phyAddr, regAddr, value);
				}
			}

			length -= 12;
			initSeq += 12;
		}
	}

	close(fd);
	return true;
}

uint32_t AxiMdio::readPhy(uint32_t phyAddr, uint32_t regAddr)
{
	if(!m_regs) return 0xFFFFFFFF;

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
	if(!m_regs) return;

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
