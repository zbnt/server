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

#include <MeasurementServer.hpp>

#include <QThread>
#include <QNetworkInterface>

#include <Utils.hpp>
#include <Settings.hpp>
#include <BitstreamManager.hpp>

MeasurementServer *g_measurementSrv = nullptr;

MeasurementServer::MeasurementServer(QObject *parent)
	: QObject(parent)
{
	m_server = new QTcpServer(this);
	m_dmaTimer = new QTimer(this);
	m_helloTimer = new QTimer(this);
	m_runEndTimer = new QTimer(this);

	m_dmaTimer->setInterval(0);
	m_dmaTimer->setSingleShot(false);
	m_dmaTimer->start();

	m_helloTimer->setInterval(MSG_HELLO_TIMEOUT);
	m_helloTimer->setSingleShot(true);

	m_runEndTimer->setInterval(1000);
	m_runEndTimer->setSingleShot(false);
	m_runEndTimer->start();

	if(!m_server->listen(QHostAddress::Any, g_daemonCfg.port))
		qFatal("[net] F: Can't listen on TCP port");

	g_daemonCfg.port = m_server->serverPort();

	qInfo("[net] I: Listening on TCP port %d", g_daemonCfg.port);

	connect(m_dmaTimer, &QTimer::timeout, this, &MeasurementServer::flushDmaBuffer);
	connect(m_helloTimer, &QTimer::timeout, this, &MeasurementServer::onHelloTimeout);
	connect(m_runEndTimer, &QTimer::timeout, this, &MeasurementServer::pollAxiTimer);
	connect(m_server, &QTcpServer::newConnection, this, &MeasurementServer::onIncomingConnection);
}

MeasurementServer::~MeasurementServer()
{ }

void MeasurementServer::startRun()
{
	if(m_isRunning) return;

	m_lastDmaIdx = 0;
	m_pendingDmaData.clear();
	g_axiDma->startTransfer();

	if(m_client)
	{
		writeMessage(m_client, MSG_ID_RUN_START, QByteArray());
	}

	m_isRunning = true;
	qInfo("[net] I: Run started");
}

void MeasurementServer::stopRun()
{
	if(!m_isRunning) return;

	g_axiTimer->setRunning(false);

	// Flush data still remaining in the FIFOs and stop DMA

	g_axiDma->flushFifo();

	while(!g_axiDma->isFifoEmpty())
	{
		QThread::usleep(100);
	}

	g_axiDma->stopTransfer();

	if(g_axiDma->waitForInterrupt(0))
	{
		g_axiDma->clearInterrupts(g_axiDma->getActiveInterrupts());
	}

	// Reset the AXI timer

	uint64_t maxTime = g_axiTimer->getMaximumTime();

	g_axiTimer->setReset(true);
	QThread::msleep(100);
	g_axiTimer->setReset(false);

	g_axiTimer->setMaximumTime(maxTime);

	// Notify client, if available

	if(m_client)
	{
		for(AbstractDevice *dev : g_deviceList)
		{
			QByteArray value;

			if(dev->getProperty(PROP_OVERFLOW_COUNT, QByteArray(), value))
			{
				QByteArray response;
				appendAsBytes<uint8_t>(response, dev->getIndex());
				appendAsBytes<uint16_t>(response, PROP_OVERFLOW_COUNT);
				appendAsBytes<uint8_t>(response, true);
				response.append(value);

				writeMessage(m_client, MSG_ID_GET_PROPERTY, response);
			}
		}

		writeMessage(m_client, MSG_ID_RUN_STOP, QByteArray());
	}

	m_isRunning = false;
	qInfo("[net] I: Run stopped");
}

void MeasurementServer::pollAxiTimer()
{
	if(g_axiTimer && g_axiDma)
	{
		if(g_axiTimer->getCurrentTime() >= g_axiTimer->getMaximumTime())
		{
			if(g_axiDma->isFifoEmpty())
			{
				qDebug("[net] I: Time limit reached");
				stopRun();
			}
			else
			{
				g_axiDma->flushFifo();
			}
		}
	}
}

void MeasurementServer::flushDmaBuffer()
{
	if(g_axiDma->waitForInterrupt(0))
	{
		const char *buffer = g_dmaBuffer->getVirtAddr();
		uint32_t bufferSize = g_dmaBuffer->getMemSize();
		uint32_t msgEnd = g_axiDma->getLastMessageEnd();
		uint16_t irq = g_axiDma->getActiveInterrupts();

		if(m_client)
		{
			m_client->write(m_pendingDmaData);
			m_client->write(buffer + m_lastDmaIdx, msgEnd - m_lastDmaIdx);
		}

		m_pendingDmaData.clear();
		m_lastDmaIdx = msgEnd;

		if(irq & AxiDma::IRQ_MEM_END)
		{
			m_lastDmaIdx = 0;

			if(msgEnd != bufferSize)
			{
				m_pendingDmaData.append(buffer + msgEnd, bufferSize - msgEnd);
			}
		}

		g_axiDma->clearInterrupts(irq);
	}
}

void MeasurementServer::onHelloTimeout()
{
	if(m_client && !m_helloReceived)
	{
		qInfo("[net] I: Client timeout, HELLO message not received");
		m_client->abort();
	}
}

void MeasurementServer::onMessageReceived(quint16 id, const QByteArray &data)
{
	switch(id)
	{
		case MSG_ID_HELLO:
		{
			if(m_helloReceived) break;

			QByteArray bitstreamList;

			for(const QString &bitName : g_bitstreamList)
			{
				const QByteArray bitNameUTF8 = bitName.toUtf8();

				appendAsBytes<uint16_t>(bitstreamList, bitNameUTF8.size());
				bitstreamList.append(bitNameUTF8);
			}

			m_helloReceived = true;
			m_helloTimer->stop();
			writeMessage(m_client, MSG_ID_HELLO, bitstreamList);

			QByteArray message;
			QByteArray activeBitstream = g_activeBitstream.toUtf8();

			appendAsBytes<uint8_t>(message, 1);
			appendAsBytes<uint16_t>(message, activeBitstream.size());
			message.append(activeBitstream);

			for(const AbstractDevice *dev : g_deviceList)
			{
				dev->announce(message);
			}

			writeMessage(m_client, MSG_ID_PROGRAM_PL, message);
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

			if(std::find(g_bitstreamList.cbegin(), g_bitstreamList.cend(), reqBitstreamName) != g_bitstreamList.cend())
			{
				loadBitstream(reqBitstreamName);
				appendAsBytes<uint8_t>(response, 1);
			}
			else
			{
				reqBitstream = g_activeBitstream.toUtf8();
				appendAsBytes<uint8_t>(response, 0);
			}

			appendAsBytes<uint16_t>(response, reqBitstream.size());
			response.append(reqBitstream);

			for(const AbstractDevice *dev : g_deviceList)
			{
				dev->announce(response);
			}

			writeMessage(m_client, MSG_ID_PROGRAM_PL, response);
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

			if(devID < g_deviceList.length())
			{
				ok = g_deviceList[devID]->setProperty(propID, value);
			}
			else if(devID == 0xFF)
			{
				ok = g_axiTimer->setProperty(propID, value);
			}

			QByteArray response;
			appendAsBytes<uint8_t>(response, devID);
			appendAsBytes<uint16_t>(response, propID);
			appendAsBytes<uint8_t>(response, ok);
			response.append(value);

			writeMessage(m_client, MSG_ID_SET_PROPERTY, response);
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

			if(devID < g_deviceList.length())
			{
				ok = g_deviceList[devID]->getProperty(propID, params, value);
			}
			else if(devID == 0xFF)
			{
				ok = g_axiTimer->getProperty(propID, params, value);
			}

			QByteArray response;
			appendAsBytes<uint8_t>(response, devID);
			appendAsBytes<uint16_t>(response, propID);
			appendAsBytes<uint8_t>(response, ok);
			response.append(params);
			response.append(value);

			writeMessage(m_client, MSG_ID_GET_PROPERTY, response);
			break;
		}

		default:
		{
			break;
		}
	}
}

void MeasurementServer::onIncomingConnection()
{
	QTcpSocket *connection = m_server->nextPendingConnection();

	if(!m_client)
	{
		m_client = connection;
		m_client->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

		qInfo("[net] I: Incoming connection: %s", qUtf8Printable(m_client->peerAddress().toString()));
		m_helloTimer->start();

		connect(m_client, &QTcpSocket::readyRead, this, &MeasurementServer::onReadyRead);
		connect(m_client, &QTcpSocket::stateChanged, this, &MeasurementServer::onNetworkStateChanged);
	}
	else
	{
		connection->abort();
		connection->deleteLater();
	}
}

void MeasurementServer::onReadyRead()
{
	handleIncomingData(m_client->readAll());
}

void MeasurementServer::onNetworkStateChanged(QAbstractSocket::SocketState state)
{
	if(state == QAbstractSocket::UnconnectedState)
	{
		qInfo("[net] I: Client disconnected");

		m_helloReceived = false;
		m_helloTimer->stop();

		m_client->deleteLater();
		m_client = nullptr;

		stopRun();
	}
}
