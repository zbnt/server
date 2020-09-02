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

#include <ZbntServer.hpp>

#include <QThread>
#include <QNetworkInterface>

#include <AbstractDevice.hpp>
#include <MessageUtils.hpp>

ZbntServer::ZbntServer(AbstractDevice *parent)
	: QObject(nullptr), m_device(parent)
{
	m_dmaTimer = new QTimer(this);
	m_helloTimer = new QTimer(this);
	m_runEndTimer = new QTimer(this);

	m_dmaTimer->setInterval(0);
	m_dmaTimer->setSingleShot(false);
	m_dmaTimer->start();

	m_helloTimer->setInterval(MSG_HELLO_TIMEOUT);
	m_helloTimer->setSingleShot(true);

	m_runEndTimer->setInterval(2000);
	m_runEndTimer->setSingleShot(false);
	m_runEndTimer->start();

	connect(m_dmaTimer, &QTimer::timeout, this, &ZbntServer::checkInterrupt);
	connect(m_helloTimer, &QTimer::timeout, this, &ZbntServer::onHelloTimeout);
	connect(m_runEndTimer, &QTimer::timeout, this, &ZbntServer::pollTimer);
}

ZbntServer::~ZbntServer()
{ }

void ZbntServer::startRun()
{
	if(m_isRunning) return;

	m_device->dmaEngine()->startTransfer();

	if(clientAvailable())
	{
		sendMessage(MSG_ID_RUN_START, QByteArray());
	}

	m_isRunning = true;
	qInfo("[net] I: Run started");
}

void ZbntServer::stopRun()
{
	if(!m_isRunning) return;

	m_device->timer()->setRunning(false);

	// Flush data still remaining in the FIFOs and stop DMA

	m_device->dmaEngine()->flushFifo();

	do
	{
		QThread::usleep(100);
		checkInterrupt();
	}
	while(!m_device->dmaEngine()->isFifoEmpty());

	m_device->dmaEngine()->stopTransfer();

	if(m_device->waitForInterrupt(0))
	{
		m_device->clearInterrupts(m_device->dmaEngine()->getActiveInterrupts());
	}

	// Reset the AXI timer

	uint64_t maxTime = m_device->timer()->getMaximumTime();

	m_device->timer()->setReset(true);
	QThread::msleep(100);
	m_device->timer()->setReset(false);

	m_device->timer()->setMaximumTime(maxTime);

	// Notify client, if available

	if(clientAvailable())
	{
		for(AbstractCore *dev : m_device->coreList())
		{
			QByteArray value;

			if(dev->getProperty(PROP_OVERFLOW_COUNT, QByteArray(), value))
			{
				QByteArray response;
				appendAsBytes<uint8_t>(response, dev->getIndex());
				appendAsBytes<uint16_t>(response, PROP_OVERFLOW_COUNT);
				appendAsBytes<uint8_t>(response, true);
				response.append(value);

				sendMessage(MSG_ID_GET_PROPERTY, response);
			}
		}

		sendMessage(MSG_ID_RUN_STOP, QByteArray());
	}

	m_isRunning = false;
	qInfo("[net] I: Run stopped");
}

void ZbntServer::onMessageReceived(quint16 id, const QByteArray &data)
{
	switch(id)
	{
		case MSG_ID_HELLO:
		{
			if(m_helloReceived) break;

			QByteArray bitstreamList;

			for(const QString &bitName : m_device->bitstreamList())
			{
				const QByteArray bitNameUTF8 = bitName.toUtf8();

				appendAsBytes<uint16_t>(bitstreamList, bitNameUTF8.size());
				bitstreamList.append(bitNameUTF8);
			}

			m_helloReceived = true;
			m_helloTimer->stop();
			sendMessage(MSG_ID_HELLO, bitstreamList);

			QByteArray message;
			QByteArray activeBitstream = m_device->activeBitstream().toUtf8();

			appendAsBytes<uint8_t>(message, 1);
			appendAsBytes<uint16_t>(message, activeBitstream.size());
			message.append(activeBitstream);

			for(const AbstractCore *dev : m_device->coreList())
			{
				dev->announce(message);
			}

			m_device->timer()->announce(message);

			sendMessage(MSG_ID_PROGRAM_PL, message);
			break;
		}

		case MSG_ID_PROGRAM_PL:
		{
			if(!m_helloReceived) break;
			if(data.length() < 3) break;

			uint16_t nameLength = readAsNumber<uint16_t>(data, 0);
			QByteArray reqBitstream = data.mid(2, nameLength);
			QString reqBitstreamName = QString::fromUtf8(reqBitstream);
			QByteArray response;

			appendAsBytes<uint8_t>(response, m_device->loadBitstream(reqBitstreamName));
			reqBitstream = m_device->activeBitstream().toUtf8();

			appendAsBytes<uint16_t>(response, reqBitstream.size());
			response.append(reqBitstream);

			for(const AbstractCore *dev : m_device->coreList())
			{
				dev->announce(response);
			}

			m_device->timer()->announce(response);

			sendMessage(MSG_ID_PROGRAM_PL, response);
			break;
		}

		case MSG_ID_RUN_START:
		{
			if(!m_helloReceived) break;

			startRun();
			break;
		}

		case MSG_ID_RUN_STOP:
		{
			if(!m_helloReceived) break;

			stopRun();
			break;
		}

		case MSG_ID_SET_PROPERTY:
		{
			if(!m_helloReceived) break;
			if(data.length() < 3) break;

			uint8_t devID = data[0];
			PropertyID propID = PropertyID(readAsNumber<uint16_t>(data, 1));
			QByteArray value = data.mid(3);
			bool ok = false;

			if(devID < m_device->coreList().length())
			{
				ok = m_device->coreList().at(devID)->setProperty(propID, value);
			}
			else if(devID == 0xFF)
			{
				ok = m_device->timer()->setProperty(propID, value);
			}

			QByteArray response;
			appendAsBytes<uint8_t>(response, devID);
			appendAsBytes<uint16_t>(response, propID);
			appendAsBytes<uint8_t>(response, ok);
			response.append(value);

			sendMessage(MSG_ID_SET_PROPERTY, response);
			break;
		}

		case MSG_ID_GET_PROPERTY:
		{
			if(!m_helloReceived) break;
			if(data.length() < 3) break;

			uint8_t devID = data[0];
			PropertyID propID = PropertyID(readAsNumber<uint16_t>(data, 1));
			QByteArray params = data.mid(3);
			QByteArray value;
			bool ok = false;

			if(devID < m_device->coreList().length())
			{
				ok = m_device->coreList().at(devID)->getProperty(propID, params, value);
			}
			else if(devID == 0xFF)
			{
				ok = m_device->timer()->getProperty(propID, params, value);
			}

			QByteArray response;
			appendAsBytes<uint8_t>(response, devID);
			appendAsBytes<uint16_t>(response, propID);
			appendAsBytes<uint8_t>(response, ok);
			response.append(params);
			response.append(value);

			sendMessage(MSG_ID_GET_PROPERTY, response);
			break;
		}

		default:
		{
			break;
		}
	}
}

void ZbntServer::checkInterrupt()
{
	if(m_device->waitForInterrupt(0))
	{
		uint8_t *buffer = m_device->dmaBuffer()->getVirtualAddr();
		uint32_t bufferSize = m_device->dmaBuffer()->getSize();
		uint32_t msgEnd = m_device->dmaEngine()->getLastMessageEnd();
		uint16_t irq = m_device->dmaEngine()->getActiveInterrupts();

		if(clientAvailable())
		{
			sendBytes(m_pendingDmaData);
			sendBytes(buffer + m_lastDmaIdx, msgEnd - m_lastDmaIdx);
		}

		m_pendingDmaData.clear();
		m_lastDmaIdx = msgEnd;

		if(irq & AxiDma::IRQ_MEM_END)
		{
			m_lastDmaIdx = 0;

			if(msgEnd != bufferSize)
			{
				m_pendingDmaData.append((char*) buffer + msgEnd, bufferSize - msgEnd);
			}
		}

		m_device->clearInterrupts(irq);
	}
}

void ZbntServer::pollTimer()
{
	if(m_device->timer() && m_device->dmaEngine())
	{
		if(m_isRunning && m_device->timer()->getCurrentTime() >= m_device->timer()->getMaximumTime())
		{
			qDebug("[net] I: Time limit reached");
			stopRun();
		}
	}
}
