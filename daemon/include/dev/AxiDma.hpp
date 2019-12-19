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
		uint32_t mm2s_dmacr;     // +0
		uint32_t mm2s_dmasr;     // +4
		uint32_t __reserved1[4]; // +8
		uint64_t mm2s_sa;        // +24
		uint32_t __reserved2[2];
		uint32_t mm2s_length;    // +40
		uint32_t __reserved3;

		uint32_t s2mm_dmacr;     // +48
		uint32_t s2mm_dmasr;     // +52
		uint32_t __reserved4[4];
		uint64_t s2mm_da;        // +72
		uint32_t __reserved5[2];
		uint32_t s2mm_length;    // +88
	};

public:
	AxiDma(const QByteArray &name);
	~AxiDma();

	DeviceType getType() const;

	bool isReady() const;
	bool loadDevice(const void *fdt, int offset);

	uint32_t waitForInterrupt();
	void clearInterrupts();
	void startTransfer();

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, QByteArray &value);

private:
	volatile Registers *m_regs;
	size_t m_regsSize;
	int m_fd;
};
