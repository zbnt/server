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

typedef struct
{
	uint32_t config;
	uint32_t padding;
	uint32_t delay;
	uint32_t timeout;
	uint32_t fifo_occupancy;
	uint32_t fifo_pop;

	uint64_t time;
	uint32_t ping_latency;
	uint32_t pong_latency;
	uint64_t ping_pong_good;
	uint64_t pings_lost;
	uint64_t pongs_lost;
} LatencyMeasurer;