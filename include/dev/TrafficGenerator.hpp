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

#pragma once

#include <cstdint>

#include <dev/AbstractDevice.hpp>

#define TGEN_MEM_FRAME_OFFSET   0x0800
#define TGEN_MEM_MASK_OFFSET    0x1000
#define TGEN_MEM_SIZE           2048

class TrafficGenerator : public AbstractDevice
{
public:
	static constexpr uint32_t CFG_ENABLE   = 1;
	static constexpr uint32_t CFG_RESET    = 2;
	static constexpr uint32_t CFG_BURST    = 4;
	static constexpr uint32_t CFG_SEED_REQ = 8;

	struct Registers
	{
		uint32_t config;
		uint32_t status;
		uint32_t fsize;
		uint32_t fdelay;
		uint16_t burst_time_on;
		uint16_t burst_time_off;
		uint8_t lfsr_seed_val;
	};

public:
	TrafficGenerator(const QByteArray &name, uint32_t index);
	~TrafficGenerator();

	void announce(QByteArray &output) const;

	DeviceType getType() const;
	uint64_t getPorts() const;

	bool isReady() const;
	bool loadDevice(const void *fdt, int offset);

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, const QByteArray &params, QByteArray &value);

private:
	volatile Registers *m_regs;
	size_t m_regsSize;

	uint8_t m_port;
};
