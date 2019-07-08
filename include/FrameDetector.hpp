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

#define FD_MEM_A_OFFSET 0x2000
#define FD_MEM_B_OFFSET 0x4000
#define FD_MEM_SIZE     1536 * 4

typedef struct
{
	uint32_t config;
	uint32_t fifo_occupancy;
	uint32_t fifo_pop;

	uint64_t time;
	uint32_t matched_patterns;
} FrameDetector;
