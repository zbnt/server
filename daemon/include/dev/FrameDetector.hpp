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

#define FD_PATTERN_A_DATA_OFFSET  0x2000
#define FD_PATTERN_A_FLAGS_OFFSET 0x4000
#define FD_PATTERN_B_DATA_OFFSET  0x6000
#define FD_PATTERN_B_FLAGS_OFFSET 0x8000
#define FD_MEM_SIZE               8192
#define FD_NUM_PATTERNS           4

class FrameDetector : public AbstractDevice
{
public:
	static constexpr uint32_t CFG_ENABLE   = 1;
	static constexpr uint32_t CFG_RESET    = 2;
	static constexpr uint32_t CFG_FIX_CSUM = 4;

	struct Registers
	{
		uint16_t config;
		uint16_t log_identifier;
		uint32_t match_enable;

		uint64_t overflow_count_a;
		uint64_t overflow_count_b;
	};

public:
	FrameDetector(const QByteArray &name, uint32_t index);
	~FrameDetector();

	DeviceType getType() const;
	uint64_t getPorts() const;

	bool isReady() const;
	bool loadDevice(const void *fdt, int offset);

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, QByteArray &value);

private:
	volatile Registers *m_regs;
	size_t m_regsSize;

	uint8_t m_portA, m_portB;
};
