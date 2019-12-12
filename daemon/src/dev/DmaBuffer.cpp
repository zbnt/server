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

#include <dev/DmaBuffer.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <Utils.hpp>
#include <BitstreamManager.hpp>

DmaBuffer::DmaBuffer(const QByteArray &name)
	: AbstractDevice(name), m_ptr(nullptr), m_memSize(0)
{ }

DmaBuffer::~DmaBuffer()
{
	if(m_ptr)
	{
		munmap((void*) m_ptr, m_memSize);
	}
}

DeviceType DmaBuffer::getType()
{
	return DEV_DMA_BUFFER;
}

uint32_t DmaBuffer::getIdentifier()
{
	return 0x80000000;
}

bool DmaBuffer::loadDevice(const void *fdt, int offset)
{
	if(!fdt || m_ptr) return false;

	QByteArray devName;

	if(!fdtGetArrayProp(fdt, offset, "size", m_memSize))
	{
		qCritical("[dev] E: Device %s doesn't have a valid size property.", m_name.constData());
		return false;
	}

	if(!fdtGetStringProp(fdt, offset, "device-name", devName))
	{
		qCritical("[dev] E: Device %s doesn't have a valid device-name property.", m_name.constData());
		return false;
	}

	qDebug("[dev] D: Found %s with name %s and size %lu bytes", m_name.constData(), devName.constData(), m_memSize);

	// Open memory map

	QByteArray devPath = "/dev/" + devName;
	int fd = open(devPath.constData(), O_RDWR | O_SYNC);

	if(fd == -1)
	{
		qCritical("[dev] E: Failed to open %s", devPath.constData());
		return false;
	}

	m_ptr = (uint8_t*) mmap(NULL, m_memSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(!m_ptr)
	{
		qCritical("[dev] E: Failed to mmap %s", devPath.constData());
		return false;
	}

	close(fd);
	return true;
}

void DmaBuffer::setReset(bool reset)
{
}

bool DmaBuffer::setProperty(const QByteArray &key, const QByteArray &value)
{
	return true;
}

bool DmaBuffer::getProperty(const QByteArray &key, QByteArray &value)
{
	return true;
}

