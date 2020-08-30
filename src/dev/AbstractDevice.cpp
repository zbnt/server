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

#include <dev/AbstractDevice.hpp>

AbstractDevice::AbstractDevice(const QByteArray &name, uint32_t index)
	: m_name(name), m_idx(index)
{ }

AbstractDevice::~AbstractDevice()
{ }

const QByteArray &AbstractDevice::getName() const
{
	return m_name;
}

uint32_t AbstractDevice::getIndex() const
{
	return m_idx;
}

uint64_t AbstractDevice::getPorts() const
{
	return 0;
}