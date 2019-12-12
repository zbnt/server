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

class FrameDetector : public AbstractDevice
{
public:
	struct Registers
	{
		uint32_t config;
		uint32_t fifo_occupancy;
		uint32_t fifo_pop;
		uint32_t _reserved;

		uint32_t time_l;
		uint32_t time_h;
		uint8_t match_dir;
		uint8_t match_mask;
		uint8_t match_ext_num;
		uint8_t _reserved2;
		uint8_t match_ext_data[16];
	};

public:
	FrameDetector(const QByteArray &name);
	~FrameDetector();

	DeviceType getType();
	uint32_t getIdentifier();

	bool loadDevice(const void *fdt, int offset);

	void setReset(bool reset);
	bool setProperty(const QByteArray &key, const QByteArray &value);
	bool getProperty(const QByteArray &key, QByteArray &value);

private:
	volatile Registers *m_regs;
	size_t m_regsSize;

	uint8_t m_portA, m_portB;
};
