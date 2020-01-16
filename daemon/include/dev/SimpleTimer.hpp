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

class SimpleTimer : public AbstractDevice
{
public:
	static constexpr uint32_t CFG_ENABLE = 1;
	static constexpr uint32_t CFG_RESET  = 2;

	struct Registers
	{
		uint32_t config;
		uint32_t status;

		uint64_t max_time;
		uint64_t current_time;
	};

public:
	SimpleTimer(const QByteArray &name);
	~SimpleTimer();

	DeviceType getType() const;

	bool isReady() const;
	bool loadDevice(const void *fdt, int offset);

	void setMaximumTime(uint64_t time);

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, QByteArray &value);
	uint64_t getCurrentTime() const;
	uint64_t getMaximumTime() const;

private:
	volatile Registers *m_regs;
	size_t m_regsSize;
};
