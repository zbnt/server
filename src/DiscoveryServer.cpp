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

#include <DiscoveryServer.hpp>

#include <QHostInfo>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include <Utils.hpp>

DiscoveryServer::DiscoveryServer(QObject *parent) : QObject(parent)
{
	m_server = new QUdpSocket(this);
	m_server->bind(QHostAddress::Broadcast, MSG_UDP_PORT);

	connect(m_server, &QUdpSocket::readyRead, this, &DiscoveryServer::onReadyRead);
}

DiscoveryServer::~DiscoveryServer()
{ }

void DiscoveryServer::onMessageReceived(quint8 id, const QByteArray &data)
{
	if(id == MSG_ID_DISCOVERY && data.length() == 8)
	{
		m_received = true;
		m_recvdTime = data;
	}
}

void DiscoveryServer::onReadyRead()
{
	while(m_server->hasPendingDatagrams())
	{
		QNetworkDatagram datagram = m_server->receiveDatagram();

		m_received = false;
		handleIncomingData(datagram.data());

		if(m_received)
		{
			QByteArray discoveryResponse, host, ip4, ip6;

			discoveryResponse.append(MSG_MAGIC_IDENTIFIER, 4);
			appendAsBytes<quint8>(&discoveryResponse, MSG_ID_DISCOVERY_RESP);

			host = QHostInfo::localHostName().toUtf8();

			if(host.length() > 255) host.resize(255);

			for(const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
			{
				if(iface.type() == QNetworkInterface::Loopback) continue;

				for(const QNetworkAddressEntry &address : iface.addressEntries())
				{
					if(address.ip().protocol() == QAbstractSocket::IPv4Protocol)
					{
						if(ip4.length() < 16)
						{
							appendAsBytes<quint32>(&ip4, address.ip().toIPv4Address());
						}
					}
					else if(ip6.length() < 64)
					{
						ip6.append((const char*) address.ip().toIPv6Address().c, 16);
					}
				}
			}

			appendAsBytes<quint16>(&discoveryResponse, host.length() + ip4.length() + ip6.length() + 4 + 3 + 8);
			appendAsBytes<quint32>(&discoveryResponse, MSG_VERSION);
			appendAsBytes<quint8>(&discoveryResponse, host.length());
			appendAsBytes<quint8>(&discoveryResponse, ip4.length() / 4);
			appendAsBytes<quint8>(&discoveryResponse, ip6.length() / 16);
			discoveryResponse.append(m_recvdTime);
			discoveryResponse.append(host);
			discoveryResponse.append(ip4);
			discoveryResponse.append(ip6);

			m_server->writeDatagram(datagram.makeReply(discoveryResponse));
		}
	}
}
