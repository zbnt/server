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
#include <WorkerThread.hpp>
#include <BitstreamManager.hpp>

MeasurementServer::MeasurementServer(QObject *parent)
	: QObject(parent)
{
	m_timer = new QTimer(this);
	m_timer->setInterval(100);
	m_timer->setSingleShot(false);
	m_timer->start();

	m_server = new QTcpServer(this);
	m_streamServer = new QTcpServer(this);

	if(!m_server->listen(QHostAddress::Any, g_daemonCfg.mainPort))
		qFatal("[net] F: Can't listen on TCP port %d", g_daemonCfg.mainPort);

	/*if(!m_streamServer->listen(QHostAddress::Any, g_daemonCfg.streamPort))
		qFatal("[net] F: Can't listen on TCP port %d", g_daemonCfg.streamPort);*/

	qInfo("[net] I: Listening on TCP ports %d and %d", g_daemonCfg.mainPort, g_daemonCfg.streamPort);

	connect(m_timer, &QTimer::timeout, this, &MeasurementServer::sendMeasurements);
	connect(m_server, &QTcpServer::newConnection, this, &MeasurementServer::onIncomingConnection);
	connect(m_streamServer, &QTcpServer::newConnection, this, &MeasurementServer::onIncomingStreamConnection);
}

MeasurementServer::~MeasurementServer()
{ }

void MeasurementServer::sendMeasurements()
{
	if(m_client)
	{
		QMutexLocker lock(&g_workerMutex);

		if(g_msgBuffer.length())
		{
			m_client->write(g_msgBuffer);
			g_msgBuffer.clear();
		}
	}
	else
	{
		QMutexLocker lock(&g_workerMutex);
		g_msgBuffer.clear();
	}
}

void MeasurementServer::onMessageReceived(quint16 id, const QByteArray &data)
{
	QMutexLocker lock(&g_workerMutex);

	switch(id)
	{
		case MSG_ID_PROGRAM_PL:
		{
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
			// TODO
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

		connect(m_client, &QTcpSocket::readyRead, this, &MeasurementServer::onReadyRead);
		connect(m_client, &QTcpSocket::stateChanged, this, &MeasurementServer::onNetworkStateChanged);
	}
	else
	{
		connection->abort();
		connection->deleteLater();
	}
}

void MeasurementServer::onIncomingStreamConnection()
{
	QTcpSocket *connection = m_streamServer->nextPendingConnection();

	if(!m_streamClient)
	{
		m_streamClient = connection;
		m_streamReadBuffer.clear();

		qInfo("[net] I: Incoming stream connection: %s", qUtf8Printable(m_client->peerAddress().toString()));

		connect(m_streamClient, &QTcpSocket::readyRead, this, &MeasurementServer::onStreamReadyRead);
		connect(m_streamClient, &QTcpSocket::stateChanged, this, &MeasurementServer::onStreamNetworkStateChanged);
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

		m_client->deleteLater();
		m_client = nullptr;
	}
}

void MeasurementServer::onStreamReadyRead()
{
	m_streamClient->readAll();

	// TODO
}

void MeasurementServer::onStreamNetworkStateChanged(QAbstractSocket::SocketState state)
{
	if(state == QAbstractSocket::UnconnectedState)
	{
		qInfo("[net] I: Stream client disconnected");

		m_streamClient->deleteLater();
		m_streamClient = nullptr;
	}
}
