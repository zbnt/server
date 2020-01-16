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
	static constexpr uint32_t CFG_ENABLE     = 1;
	static constexpr uint32_t CFG_RESET      = 2;
	static constexpr uint32_t CFG_FLUSH_FIFO = 4;

	static constexpr uint32_t ST_OKAY        = 1;
	static constexpr uint32_t ST_ERROR_SLV   = 2;
	static constexpr uint32_t ST_ERROR_DEC   = 4;
	static constexpr uint32_t ST_FIFO_EMPTY  = 8;

	static constexpr uint32_t IRQ_MEM_END    = 1;
	static constexpr uint32_t IRQ_TIMEOUT    = 2;
	static constexpr uint32_t IRQ_AXI_ERROR  = 4;
	static constexpr uint32_t IRQ_ALL        = IRQ_MEM_END | IRQ_TIMEOUT | IRQ_AXI_ERROR;

	struct Registers
	{
		uint16_t config;
		uint16_t status;
		uint16_t irq;
		uint16_t irq_enable;
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
	void clearInterrupts(uint16_t irq);
	void startTransfer();
	void stopTransfer();
	void flushFifo();

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, QByteArray &value);
	uint16_t getActiveInterrupts() const;
	uint32_t getLastMessageEnd() const;
	uint32_t getBytesWritten() const;
	bool isFifoEmpty() const;

private:
	volatile Registers *m_regs;
	size_t m_regsSize;
	uint32_t m_irq;
	int m_fd;
};
