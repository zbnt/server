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

#include <cores/AbstractCore.hpp>

class LatencyMeasurer : public AbstractCore
{
public:
	static constexpr uint32_t CFG_ENABLE     = 1;
	static constexpr uint32_t CFG_RESET      = 2;
	static constexpr uint32_t CFG_HOLD       = 4;
	static constexpr uint32_t CFG_LOG_ENABLE = 8;
	static constexpr uint32_t CFG_BROADCAST  = 16;

	struct Registers
	{
		uint16_t config;
		uint16_t log_identifier;

		uint8_t mac_addr_a[6];
		uint8_t mac_addr_b[6];
		uint32_t ip_addr_a;
		uint32_t ip_addr_b;

		uint32_t padding;
		uint32_t delay;
		uint32_t timeout;
		uint32_t _reserved;
		uint64_t overflow_count;

		uint64_t ping_pong_good;
		uint32_t ping_latency;
		uint32_t pong_latency;
		uint64_t pings_lost;
		uint64_t pongs_lost;
	};

public:
	LatencyMeasurer(const QString &name, uint32_t id, void *regs, uint8_t portA, uint8_t portB);
	~LatencyMeasurer();

	static AbstractCore *createCore(AbstractDevice *parent, const QString &name, uint32_t id,
	                                void *regs, const void *fdt, int offset);

	void announce(QByteArray &output) const;

	DeviceType getType() const;
	uint64_t getPorts() const;

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, const QByteArray &params, QByteArray &value);

private:
	volatile Registers *m_regs;
	uint8_t m_portA, m_portB;
};
