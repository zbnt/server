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

#include <ZbntTcpServer.hpp>

#include <QThread>
#include <QNetworkInterface>

#include <MessageUtils.hpp>

ZbntTcpServer::ZbntTcpServer(quint16 port, AbstractDevice *parent)
	: ZbntServer(parent)
{
	m_server = new QTcpServer(this);

	if(!m_server->listen(QHostAddress::Any, port))
		qFatal("[net] F: Can't listen on TCP port");

	qInfo("[net] I: Listening on TCP port %d", m_server->serverPort());

	for(const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
	{
		switch(iface.type())
		{
			case QNetworkInterface::Ethernet:
			case QNetworkInterface::Wifi:
			case QNetworkInterface::Virtual:
			{
				m_discoveryServers.append(new DiscoveryServer(iface, m_server->serverPort()));
				m_discoveryServers.append(new DiscoveryServer(iface, m_server->serverPort(), true));
			}

			default: { }
		}
	}

	connect(m_server, &QTcpServer::newConnection, this, &ZbntTcpServer::onIncomingConnection);
}

ZbntTcpServer::~ZbntTcpServer()
{ }

bool ZbntTcpServer::clientAvailable() const
{
	return m_client != nullptr;
}

void ZbntTcpServer::sendBytes(const QByteArray &data)
{
	m_client->write(data);
}

void ZbntTcpServer::sendBytes(const uint8_t *data, int size)
{
	m_client->write((const char*) data, size);
}

void ZbntTcpServer::sendMessage(MessageID id, const QByteArray &data)
{
	writeMessage(m_client, id, data);
}

void ZbntTcpServer::onIncomingConnection()
{
	QTcpSocket *connection = m_server->nextPendingConnection();

	if(!m_client)
	{
		m_client = connection;
		m_client->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

		qInfo("[net] I: Incoming connection: %s", qUtf8Printable(m_client->peerAddress().toString()));
		m_helloTimer->start();

		connect(m_client, &QTcpSocket::readyRead, this, &ZbntTcpServer::onReadyRead);
		connect(m_client, &QTcpSocket::stateChanged, this, &ZbntTcpServer::onNetworkStateChanged);
	}
	else
	{
		connection->abort();
		connection->deleteLater();
	}
}

void ZbntTcpServer::onReadyRead()
{
	handleIncomingData(m_client->readAll());
}

void ZbntTcpServer::onHelloTimeout()
{
	if(m_client && !m_helloReceived)
	{
		qInfo("[net] I: Client timeout, HELLO message not received");
		m_client->abort();
	}
}

void ZbntTcpServer::onNetworkStateChanged(QAbstractSocket::SocketState state)
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
