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

#include <dev/FrameDetector.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <Utils.hpp>
#include <BitstreamManager.hpp>

FrameDetector::FrameDetector(const QByteArray &name)
	: AbstractDevice(name), m_regs(nullptr), m_regsSize(0), m_portA(0), m_portB(0)
{ }

FrameDetector::~FrameDetector()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}
}

DeviceType FrameDetector::getType()
{
	return DEV_FRAME_DETECTOR;
}

uint32_t FrameDetector::getIdentifier()
{
	return m_portA | (m_portB << 8);
}

bool FrameDetector::loadDevice(const void *fdt, int offset)
{
	if(!fdt || m_regs) return false;

	quintptr base;

	if(!fdtGetArrayProp(fdt, offset, "reg", base, m_regsSize))
	{
		qCritical("[dev] E: Device %s doesn't have a valid reg property.", m_name.constData());
		return false;
	}

	if(!fdtGetArrayProp(fdt, offset, "zbnt,ports", m_portA, m_portB))
	{
		qCritical("[dev] E: Device %s doesn't have a valid zbnt,ports property.", m_name.constData());
		return false;
	}

	// Find UIO device

	auto it = g_uioMap.find(m_name);

	if(it == g_uioMap.end())
	{
		qCritical("[dev] E: No UIO device found for %s", m_name.constData());
		return false;
	}

	qDebug("[dev] D: Found %s in %s, connected to eth%u and eth%u.", m_name.constData(), it->constData(), m_portA, m_portB);

	// Open memory map

	QByteArray uioDevice = "/dev/" + *it;
	int fd = open(uioDevice.constData(), O_RDWR | O_SYNC);

	if(fd == -1)
	{
		qCritical("[dev] E: Failed to open %s", uioDevice.constData());
		return false;
	}

	m_regs = (volatile Registers*) mmap(NULL, m_regsSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(!m_regs)
	{
		qCritical("[dev] E: Failed to mmap %s", uioDevice.constData());
		return false;
	}

	close(fd);
	return true;
}

void FrameDetector::setReset(bool reset)
{
}

bool FrameDetector::setProperty(const QByteArray &key, const QByteArray &value)
{
	return true;
}

bool FrameDetector::getProperty(const QByteArray &key, QByteArray &value)
{
	return true;
}
