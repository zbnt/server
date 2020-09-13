/*
	zbnt/server
	Copyright (C) 2020 Oscar R.

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

#include <AbstractDevice.hpp>

AbstractDevice::AbstractDevice()
{ }

AbstractDevice::~AbstractDevice()
{ }

SimpleTimer *AbstractDevice::timer() const
{
	return m_timer;
}

AxiDma *AbstractDevice::dmaEngine() const
{
	return m_dmaEngine;
}

const DmaBuffer *AbstractDevice::dmaBuffer() const
{
	return m_dmaBuffer;
}

const CoreList &AbstractDevice::coreList() const
{
	return m_coreList;
}
