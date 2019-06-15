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

#define MSG_VERSION           2
#define MSG_TCP_PORT   	      5465
#define MSG_MAGIC_IDENTIFIER  "\x4D\x60\x64\x5A"

enum MessageID
{
	MSG_ID_START,
	MSG_ID_STOP,
	MSG_ID_CFG_TG0,
	MSG_ID_CFG_TG1,
	MSG_ID_CFG_LM0,
	MSG_ID_HEADERS_TG0,
	MSG_ID_HEADERS_TG1,

	MSG_ID_MEASUREMENTS,
	MSG_ID_MEASUREMENTS_END
};

enum RxStatus
{
	MSG_RX_MAGIC,
	MSG_RX_HEADER,
	MSG_RX_DATA
};
