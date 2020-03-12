/*
	zbnt_sw
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

#pragma once

#include <cstdint>

#include <dev/AbstractDevice.hpp>

class AxiMdio : public AbstractDevice
{
public:
	static constexpr uint32_t CTL_START      = 1;
	static constexpr uint32_t CTL_ENABLE     = 8;

	static constexpr uint32_t OP_READ        = 1024;

	struct Registers
	{
		uint8_t _padding[0x7e3];
		uint32_t addr;
		uint32_t wr_data;
		uint32_t rd_data;
		uint32_t ctl;
	};

public:
	AxiMdio(const QByteArray &name, uint32_t index);
	~AxiMdio();

	void announce(QByteArray &output) const;

	DeviceType getType() const;

	bool isReady() const;
	bool loadDevice(const void *fdt, int offset);

	uint32_t readPhy(uint32_t phyAddr, uint32_t regAddr);
	uint32_t readPhyIndirect(uint32_t phyAddr, uint32_t regAddr);
	void writePhy(uint32_t phyAddr, uint32_t regAddr, uint32_t value);
	void writePhyIndirect(uint32_t phyAddr, uint32_t regAddr, uint32_t value);

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, const QByteArray &params, QByteArray &value);

private:
	volatile Registers *m_regs;
	size_t m_regsSize;

	QList<uint8_t> m_ports;
	QList<uint8_t> m_phys;
};
