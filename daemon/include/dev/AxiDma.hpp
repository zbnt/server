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

#pragma once

#include <cstdint>

#include <dev/AbstractDevice.hpp>

class AxiDma : public AbstractDevice
{
public:
	struct Registers
	{
		uint16_t config;
		uint16_t status;
		uint32_t irq;
		uint64_t mem_base;
		uint32_t mem_size;
		uint32_t bytes_written;
		uint32_t last_msg_end;
	};

public:
	AxiDma(const QByteArray &name);
	~AxiDma();

	DeviceType getType() const;

	bool isReady() const;
	bool loadDevice(const void *fdt, int offset);

	bool waitForInterrupt(int timeout);
	void clearInterrupts();
	void startTransfer();

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, QByteArray &value);
	uint32_t getLastMessageEnd();

private:
	volatile Registers *m_regs;
	size_t m_regsSize;
	uint32_t m_irq;
	int m_fd;
};
