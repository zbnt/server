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

#define MSG_VERSION           20190926
#define MSG_TCP_PORT   	      5465
#define MSG_UDP_PORT          5466
#define MSG_TCP_STREAM_PORT   5467
#define MSG_MAGIC_IDENTIFIER  "\x4D\x60\x64\x5A"

enum MessageID
{
	MSG_ID_DISCOVERY = 1,
	MSG_ID_DISCOVERY_RESP,

	MSG_ID_START,
	MSG_ID_START_STREAM,
	MSG_ID_STOP,
	MSG_ID_SET_BITSTREAM,

	MSG_ID_TG_CFG,
	MSG_ID_TG_FRAME,
	MSG_ID_TG_PATTERN,
	MSG_ID_LM_CFG,
	MSG_ID_FD_CFG,
	MSG_ID_FD_PATTERNS,

	MSG_ID_MEASUREMENT_LM,
	MSG_ID_MEASUREMENT_FD,
	MSG_ID_MEASUREMENT_SC,
	MSG_ID_DONE
};

enum RxStatus
{
	MSG_RX_MAGIC,
	MSG_RX_HEADER,
	MSG_RX_DATA
};
