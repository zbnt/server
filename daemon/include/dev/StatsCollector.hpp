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

class StatsCollector : public AbstractDevice
{
public:
	static constexpr uint32_t CFG_ENABLE     = 1;
	static constexpr uint32_t CFG_RESET      = 2;
	static constexpr uint32_t CFG_HOLD       = 4;
	static constexpr uint32_t CFG_LOG_ENABLE = 8;

	struct Registers
	{
		uint16_t config;
		uint16_t log_identifier;
		uint32_t sample_period;
		uint64_t overflow_count;

		uint64_t time;
		uint64_t tx_bytes;
		uint64_t tx_good;
		uint64_t tx_bad;
		uint64_t rx_bytes;
		uint64_t rx_good;
		uint64_t rx_bad;
	};

public:
	StatsCollector(const QByteArray &name, uint32_t index);
	~StatsCollector();

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
	uint8_t m_port;
};
