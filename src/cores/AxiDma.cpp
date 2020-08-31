/*
	zbnt/server
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

#include <cores/AxiDma.hpp>

#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <QDebug>

#include <FdtUtils.hpp>
#include <BitstreamManager.hpp>

#include <cores/DmaBuffer.hpp>

AxiDma::AxiDma(const QByteArray &name)
	: AbstractCore(name, 0x80000000), m_regs(nullptr), m_regsSize(0), m_irq(0), m_fd(-1)
{ }

AxiDma::~AxiDma()
{
	if(m_regs)
	{
		munmap((void*) m_regs, m_regsSize);
	}

	if(m_fd != -1)
	{
		close(m_fd);
	}
}

void AxiDma::announce(QByteArray &output) const
{
	Q_UNUSED(output);
}

DeviceType AxiDma::getType() const
{
	return DEV_AXI_DMA;
}

bool AxiDma::isReady() const
{
	return m_regs;
}

bool AxiDma::loadDevice(const void *fdt, int offset)
{
	if(!fdt || m_regs) return false;

	quintptr base;

	if(!fdtGetArrayProp(fdt, offset, "reg", base, m_regsSize))
	{
		qCritical("[dev] E: Device %s doesn't have a valid reg property.", m_name.constData());
		return false;
	}

	// Find UIO device

	auto it = g_uioMap.find(m_name);

	if(it == g_uioMap.end())
	{
		qCritical("[dev] E: No UIO device found for %s", m_name.constData());
		return false;
	}

	qDebug("[dev] D: Found %s in %s.", m_name.constData(), it->constData());

	// Open memory map

	QByteArray uioDevice = "/dev/" + *it;
	m_fd = open(uioDevice.constData(), O_RDWR | O_SYNC);

	if(m_fd == -1)
	{
		qCritical("[dev] E: Failed to open %s", uioDevice.constData());
		return false;
	}

	m_regs = (volatile Registers*) mmap(NULL, m_regsSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);

	if(!m_regs || m_regs == MAP_FAILED)
	{
		m_regs = nullptr;
		qCritical("[dev] E: Failed to mmap %s", uioDevice.constData());
		return false;
	}

	// Link to a buffer

	if(!g_dmaBuffer->isReady())
	{
		qCritical("[dev] E: No valid DMA buffer was found for %s", m_name.constData());
		return false;
	}

	return true;
}

bool AxiDma::waitForInterrupt(int timeout)
{
	pollfd pfd;
	pfd.fd = m_fd;
	pfd.events = POLLIN;

	if(poll(&pfd, 1, timeout) >= 1)
	{
		read(m_fd, &m_irq, sizeof(uint32_t));
		return true;
	}

	return false;
}

void AxiDma::clearInterrupts(uint16_t irq)
{
	m_regs->irq = irq;
	write(m_fd, &m_irq, sizeof(uint32_t));
}

void AxiDma::startTransfer()
{
	m_regs->mem_base = g_dmaBuffer->getPhysAddr();
	m_regs->mem_size = g_dmaBuffer->getMemSize();
	m_regs->irq_enable = IRQ_MEM_END | IRQ_TIMEOUT | IRQ_AXI_ERROR;
	m_regs->config = CFG_ENABLE;
}

void AxiDma::stopTransfer()
{
	m_regs->config &= ~CFG_ENABLE;
}

void AxiDma::flushFifo()
{
	m_regs->config |= CFG_FLUSH_FIFO;
}

void AxiDma::setReset(bool reset)
{
	if(reset)
	{
		m_regs->config |= CFG_RESET;
	}
	else
	{
		m_regs->config &= ~CFG_RESET;
	}
}

bool AxiDma::setProperty(PropertyID propID, const QByteArray &value)
{
	Q_UNUSED(propID);
	Q_UNUSED(value);
	return false;
}

bool AxiDma::getProperty(PropertyID propID, const QByteArray &params, QByteArray &value)
{
	Q_UNUSED(propID);
	Q_UNUSED(params);
	Q_UNUSED(value);
	return false;
}

uint16_t AxiDma::getActiveInterrupts() const
{
	return m_regs->irq;
}

uint32_t AxiDma::getLastMessageEnd() const
{
	return m_regs->last_msg_end;
}

uint32_t AxiDma::getBytesWritten() const
{
	return m_regs->bytes_written;
}

bool AxiDma::isFifoEmpty() const
{
	return !!(m_regs->status & ST_FIFO_EMPTY);
}
