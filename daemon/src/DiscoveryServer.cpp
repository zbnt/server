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
#include <Settings.hpp>

DiscoveryServer::DiscoveryServer(QObject *parent) : QObject(parent)
{
	m_server = new QUdpSocket(this);
	connect(m_server, &QUdpSocket::readyRead, this, &DiscoveryServer::onReadyRead);

	QHostAddress broadcastAddr;

	for(const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
	{
		if(iface.type() == QNetworkInterface::Loopback) continue;

		for(const QNetworkAddressEntry &address : iface.addressEntries())
		{
			if(address.ip().protocol() == QAbstractSocket::IPv4Protocol)
			{
				if(broadcastAddr.isNull() || broadcastAddr.isLinkLocal())
				{
					broadcastAddr = address.broadcast();
				}
			}
		}
	}

	if(!broadcastAddr.isNull())
	{
		m_server->bind(broadcastAddr, MSG_DISCOVERY_PORT);
	}
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
			QByteArray discoveryResponse, host;
			QHostAddress ip4, ip6;

			host = QHostInfo::localHostName().toUtf8();

			if(host.length() > 255) host.resize(255);

			for(const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
			{
				if(iface.type() == QNetworkInterface::Loopback) continue;

				for(const QNetworkAddressEntry &address : iface.addressEntries())
				{
					if(address.ip().protocol() == QAbstractSocket::IPv4Protocol)
					{
						if(ip4.isNull() || ip4.isLinkLocal())
						{
							ip4 = address.ip();
						}
					}
					else
					{
						if(ip6.isNull() && !ip6.isLinkLocal())
						{
							ip6 = address.ip();
						}
					}
				}
			}

			if(ip4.isNull() && ip6.isNull()) continue;

			discoveryResponse.append(MSG_MAGIC_IDENTIFIER, 4);
			appendAsBytes<quint8>(discoveryResponse, MSG_ID_DISCOVERY);
			appendAsBytes<quint16>(discoveryResponse, 4 + 8 + 4 + 16 + 4 + host.length());
			appendAsBytes<quint32>(discoveryResponse, ZBNT_VERSION_INT);
			discoveryResponse.append(m_recvdTime);

			if(!ip4.isNull())
			{
				appendAsBytes<quint32>(discoveryResponse, ip4.toIPv4Address());
			}
			else
			{
				discoveryResponse.append(4, '\0');
			}

			if(!ip6.isNull())
			{
				discoveryResponse.append((const char*) ip6.toIPv6Address().c, 16);
			}
			else
			{
				discoveryResponse.append(16, '\0');
			}

			appendAsBytes<quint16>(discoveryResponse, g_daemonCfg.mainPort);
			appendAsBytes<quint16>(discoveryResponse, g_daemonCfg.streamPort);

			discoveryResponse.append(host);

			m_server->writeDatagram(datagram.makeReply(discoveryResponse));
		}
	}
}
