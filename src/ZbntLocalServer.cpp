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

#include <ZbntLocalServer.hpp>

#include <sys/socket.h>
#include <sys/un.h>

#include <QCoreApplication>

#include <MessageUtils.hpp>

ZbntLocalServer::ZbntLocalServer(AbstractDevice *parent)
	: ZbntServer(parent)
{
	m_server = new QLocalServer(this);

	qintptr sock = socket(AF_UNIX, SOCK_STREAM, 0);
	qint64 pid = QCoreApplication::applicationPid();

	if(sock == -1)
		qFatal("[net] F: Can't create local socket");

	sockaddr_un sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	snprintf(sockAddr.sun_path + 1, sizeof(sockAddr.sun_path) - 1, "/tmp/zbnt-local-%08llX", pid);
	sockAddr.sun_family = AF_UNIX;

	if(bind(sock, (sockaddr*) &sockAddr, sizeof(sa_family_t) + 25) == -1)
		qFatal("[net] F: Can't bind local socket");

	if(listen(sock, m_server->maxPendingConnections()) == -1)
		qFatal("[net] F: Can't listen on local socket");

	if(!m_server->listen(sock))
		qFatal("[net] F: Can't listen on local socket");

	qInfo("[net] I: Listening on %s", qUtf8Printable(m_server->serverName()));

	connect(m_server, &QLocalServer::newConnection, this, &ZbntLocalServer::onIncomingConnection);
}

ZbntLocalServer::~ZbntLocalServer()
{ }

bool ZbntLocalServer::clientAvailable() const
{
	return m_client != nullptr;
}

void ZbntLocalServer::sendBytes(const QByteArray &data)
{
	m_client->write(data);
}

void ZbntLocalServer::sendBytes(const uint8_t *data, int size)
{
	m_client->write((const char*) data, size);
}

void ZbntLocalServer::sendMessage(MessageID id, const QByteArray &data)
{
	writeMessage(m_client, id, data);
}

void ZbntLocalServer::onIncomingConnection()
{
	QLocalSocket *connection = m_server->nextPendingConnection();

	if(!m_client)
	{
		m_client = connection;

		qInfo("[net] I: Incoming connection");
		m_helloTimer->start();

		connect(m_client, &QLocalSocket::readyRead, this, &ZbntLocalServer::onReadyRead);
		connect(m_client, &QLocalSocket::stateChanged, this, &ZbntLocalServer::onNetworkStateChanged);
	}
	else
	{
		connection->abort();
		connection->deleteLater();
	}
}

void ZbntLocalServer::onReadyRead()
{
	handleIncomingData(m_client->readAll());
}

void ZbntLocalServer::onHelloTimeout()
{
	if(m_client && !m_helloReceived)
	{
		qInfo("[net] I: Client timeout, HELLO message not received");
		m_client->abort();
	}
}

void ZbntLocalServer::onNetworkStateChanged(QLocalSocket::LocalSocketState state)
{
	if(state == QLocalSocket::UnconnectedState)
	{
		qInfo("[net] I: Client disconnected");

		m_helloReceived = false;
		m_helloTimer->stop();

		m_client->deleteLater();
		m_client = nullptr;

		stopRun();
	}
}
