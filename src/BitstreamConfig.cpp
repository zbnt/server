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

#include <BitstreamConfig.hpp>

#include <QFile>

uint8_t bitstream = BITSTREAM_NONE;

bool programPL(BitstreamID bid)
{
	QFile plDevice("/dev/xdevcfg");
	QFile srcBitstream;

	switch(bid)
	{
		case BITSTREAM_DUAL_TGEN:
		{
			srcBitstream.setFileName(":/bd_dual_tgen.bin");
			break;
		}

		case BITSTREAM_QUAD_TGEN:
		{
			srcBitstream.setFileName(":/bd_quad_tgen.bin");
			break;
		}

		default: return false;
	}

	if(!plDevice.open(QIODevice::WriteOnly)) return false;
	if(!srcBitstream.open(QIODevice::ReadOnly)) return false;

	plDevice.write(srcBitstream.readAll());
	plDevice.close();
	srcBitstream.close();

	bitstream = bid;
	return true;
}
