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

#include <QDebug>

#include <AbstractDevice.hpp>
#include <FdtUtils.hpp>

AxiDma::AxiDma(const QString &name, uint32_t id, void *regs, const DmaBuffer *buffer)
	: AbstractCore(name, id), m_regs((volatile Registers*) regs), m_buffer(buffer)
{
	m_regs->mem_base = buffer->getPhysicalAddr();
	m_regs->mem_size = buffer->getSize();
}

AxiDma::~AxiDma()
{ }

AbstractCore *AxiDma::createCore(AbstractDevice *parent, const QString &name, uint32_t id,
                                 void *regs, const void *fdt, int offset)
{
	Q_UNUSED(fdt);
	Q_UNUSED(offset);

	if(!parent->dmaBuffer())
	{
		qCritical("[core] E: No valid DMA buffer found");
		return nullptr;
	}

	return new AxiDma(name, id, regs, parent->dmaBuffer());
}

void AxiDma::announce(QByteArray &output) const
{
	Q_UNUSED(output);
}

DeviceType AxiDma::getType() const
{
	return DEV_AXI_DMA;
}

void AxiDma::clearInterrupts(uint16_t irq)
{
	m_regs->irq = irq;
}

void AxiDma::startTransfer()
{
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
