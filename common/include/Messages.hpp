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

#define MSG_MAGIC_IDENTIFIER "\xFFZB\x02"
#define MSG_DISCOVERY_PORT   5466
#define MSG_HELLO_TIMEOUT    3000

#include <QObject>

namespace Messages
{
	Q_NAMESPACE

	enum MessageID
	{
		// Added in 2.0.0

		MSG_ID_DISCOVERY = 1,
		MSG_ID_HELLO,
		MSG_ID_PROGRAM_PL,
		MSG_ID_TIME_OVER,
		MSG_ID_SET_PROPERTY,
		MSG_ID_GET_PROPERTY,
		MSG_ID_USER_MESSAGE,

		MSG_ID_MEASUREMENT = 0x8000
	};

	enum PropertyID
	{
		// Added in 2.0.0

		PROP_ENABLE = 1,
		PROP_ENABLE_LOG,
		PROP_ENABLE_BURST,
		PROP_ENABLE_CSUM_FIX,
		PROP_ENABLE_PATTERN,
		PROP_ENABLE_BROADCAST,

		PROP_TIMER_MODE,
		PROP_TIMER_TIME,
		PROP_TIMER_LIMIT,

		PROP_FRAME_SIZE,
		PROP_FRAME_GAP,
		PROP_FRAME_PADDING,
		PROP_FRAME_TEMPLATE,
		PROP_FRAME_PATTERN,
		PROP_FRAME_PATTERN_FLAGS,

		PROP_BURST_TIME_ON,
		PROP_BURST_TIME_OFF,

		PROP_MAC_ADDR,
		PROP_IP_ADDR,
		PROP_TIMEOUT,

		PROP_OVERFLOW_COUNT,
		PROP_SAMPLE_PERIOD,
		PROP_LFSR_SEED
	};

	enum RxStatus
	{
		MSG_RX_MAGIC,
		MSG_RX_HEADER,
		MSG_RX_DATA
	};

	enum DeviceType
	{
		DEV_AXI_DMA = 1,
		DEV_DMA_BUFFER,
		DEV_SIMPLE_TIMER,
		DEV_FRAME_DETECTOR,
		DEV_STATS_COLLECTOR,
		DEV_LATENCY_MEASURER,
		DEV_TRAFFIC_GENERATOR
	};

	Q_ENUM_NS(MessageID)
	Q_ENUM_NS(PropertyID)
	Q_ENUM_NS(DeviceType)
}

using namespace Messages;
