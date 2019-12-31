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

	m_dmaTimer->setInterval(0);
	m_dmaTimer->setSingleShot(false);
	m_dmaTimer->start();

	m_helloTimer->setInterval(MSG_HELLO_TIMEOUT);
	m_helloTimer->setSingleShot(true);

	if(!m_server->listen(QHostAddress::Any, g_daemonCfg.port))
		qFatal("[net] F: Can't listen on TCP port");

	g_daemonCfg.port = m_server->serverPort();

	qInfo("[net] I: Listening on TCP port %d", g_daemonCfg.port);

	connect(m_dmaTimer, &QTimer::timeout, this, &MeasurementServer::flushDmaBuffer);
	connect(m_helloTimer, &QTimer::timeout, this, &MeasurementServer::onHelloTimeout);
	connect(m_server, &QTcpServer::newConnection, this, &MeasurementServer::onIncomingConnection);
}

MeasurementServer::~MeasurementServer()
{ }

void MeasurementServer::flushDmaBuffer()
{
	if(g_axiDma->waitForInterrupt(0))
	{
		const char *buffer = g_dmaBuffer->getVirtAddr();
		uint32_t bufferSize = g_dmaBuffer->getMemSize();
		uint32_t msgEnd = g_axiDma->getLastMessageEnd();

		if(m_client)
		{
			m_client->write(m_pendingDmaData);
			m_client->write(buffer, msgEnd);
		}

		m_pendingDmaData.clear();

		if(msgEnd != bufferSize)
		{
			m_pendingDmaData.append(buffer + msgEnd, bufferSize - msgEnd);
		}

		g_axiDma->clearInterrupts();
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
			appendAsBytes<uint16_t>(bitstreamList, g_bitstreamList.size());

			for(const QString &bitName : g_bitstreamList)
			{
				const QByteArray bitNameUTF8 = bitName.toUtf8();

				appendAsBytes<uint16_t>(bitstreamList, bitNameUTF8.size());
				bitstreamList.append(bitNameUTF8);

				QByteArray deviceList;
				uint32_t deviceCount = enumerateDevices(bitName, deviceList);

				appendAsBytes<uint16_t>(bitstreamList, deviceCount);
				bitstreamList.append(deviceList);
			}

			m_helloReceived = true;
			m_helloTimer->stop();

			writeMessage(m_client, MSG_ID_HELLO, bitstreamList);
			break;
		}

		case MSG_ID_PROGRAM_PL:
		{
			if(!m_helloReceived) break;
			if(!data.length()) break;

			QString bitstreamName(data);

			if(std::find(g_bitstreamList.cbegin(), g_bitstreamList.cend(), bitstreamName) != g_bitstreamList.cend())
			{
				loadBitstream(bitstreamName);
			}

			break;
		}

		case MSG_ID_SET_PROPERTY:
		{
			if(!m_helloReceived) break;
			if(data.length() < 3) break;

			uint8_t devID = data[0];
			PropertyID propID = PropertyID(readAsNumber<uint16_t>(data, 1));
			QByteArray value = data.mid(3);

			if(devID < g_deviceList.length())
			{
				g_deviceList[devID]->setProperty(propID, value);
			}
			else if(devID == 0xFF)
			{
				g_axiTimer->setProperty(propID, value);
			}

			break;
		}

		case MSG_ID_GET_PROPERTY:
		{
			if(!m_helloReceived) break;
			if(data.length() < 3) break;

			uint8_t devID = data[0];
			PropertyID propID = PropertyID(readAsNumber<uint16_t>(data, 1));
			QByteArray value;
			bool ok = false;

			if(devID < g_deviceList.length())
			{
				ok = g_deviceList[devID]->getProperty(propID, value);
			}
			else if(devID == 0xFF)
			{
				ok = g_axiTimer->getProperty(propID, value);
			}

			QByteArray response;
			appendAsBytes<uint8_t>(response, ok);
			response.append(value);

			writeAsBytes(m_client, response);
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
	}
}
