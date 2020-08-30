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

#include <Version.hpp>
#include <Settings.hpp>
#include <MessageUtils.hpp>
#include <MeasurementServer.hpp>

QVector<DiscoveryServer*> g_discoverySrv;

DiscoveryServer::DiscoveryServer(const QNetworkInterface &iface, bool ip6, QObject *parent)
	: QObject(parent), m_ip6(ip6), m_iface(iface)
{
	m_server = new QUdpSocket(this);
	connect(m_server, &QUdpSocket::readyRead, this, &DiscoveryServer::onReadyRead);

	if(!ip6)
	{
		QHostAddress broadcastAddr;

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

		if(!broadcastAddr.isNull())
		{
			m_server->bind(broadcastAddr, MSG_DISCOVERY_PORT);
		}
	}
	else
	{
		QHostAddress multicastAddr;
		multicastAddr.setAddress(QString("ff12::%1").arg(MSG_DISCOVERY_PORT));
		multicastAddr.setScopeId(iface.name());

		m_server->bind(multicastAddr, MSG_DISCOVERY_PORT);
		m_server->joinMulticastGroup(multicastAddr, iface);
	}
}

DiscoveryServer::~DiscoveryServer()
{ }

void DiscoveryServer::onReadyRead()
{
	while(m_server->hasPendingDatagrams())
	{
		QNetworkDatagram datagram = m_server->receiveDatagram();
		QByteArray rx_message = datagram.data();

		if(rx_message.size() != 16)
		{
			continue;
		}

		if(!rx_message.startsWith(MSG_MAGIC_IDENTIFIER))
		{
			continue;
		}

		uint16_t messageID = readAsNumber<uint16_t>(rx_message, 4);
		uint16_t messageSize = readAsNumber<uint16_t>(rx_message, 6);

		if(messageID != MSG_ID_DISCOVERY || messageSize != 8)
		{
			continue;
		}

		rx_message.remove(0, 8);

		QByteArray discoveryResponse, host;

		host = QHostInfo::localHostName().toUtf8();
		if(host.length() > 255) host.resize(255);

		discoveryResponse.append(MSG_MAGIC_IDENTIFIER, 4);
		appendAsBytes<quint16>(discoveryResponse, MSG_ID_DISCOVERY);
		appendAsBytes<quint16>(discoveryResponse, 8 + 4 + 16 + 16 + 1 + 2 + host.length());

		discoveryResponse.append(rx_message);

		appendAsBytes<quint32>(discoveryResponse, ZBNT_VERSION_INT);
		discoveryResponse.append(padString(ZBNT_VERSION_PREREL, 16));
		discoveryResponse.append(padString(ZBNT_VERSION_COMMIT, 16));
		appendAsBytes<quint8>(discoveryResponse, ZBNT_VERSION_DIRTY);

		appendAsBytes<quint16>(discoveryResponse, g_daemonCfg.port);

		discoveryResponse.append(host);

		m_server->writeDatagram(datagram.makeReply(discoveryResponse));
	}
}
