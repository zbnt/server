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

#pragma once

#include <QTcpServer>
#include <QTcpSocket>

#include <DiscoveryServer.hpp>
#include <ZbntServer.hpp>

class ZbntTcpServer : public ZbntServer
{
public:
	ZbntTcpServer(const QHostAddress &address, quint16 port, AbstractDevice *parent);
	~ZbntTcpServer();

private:
	bool clientAvailable() const;
	void sendBytes(const QByteArray &data);
	void sendBytes(const uint8_t *data, int size);
	void sendMessage(MessageID id, const QByteArray &data);

	void onIncomingConnection();
	void onReadyRead();
	void onHelloTimeout();
	void onNetworkStateChanged(QAbstractSocket::SocketState state);

private:
	QTcpServer *m_server = nullptr;
	QTcpSocket *m_client = nullptr;
	QVector<DiscoveryServer*> m_discoveryServers;
};
