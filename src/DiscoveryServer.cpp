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

#include <DiscoveryServer.hpp>

#include <QCoreApplication>
#include <QHostInfo>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include <Version.hpp>
#include <MessageUtils.hpp>

DiscoveryServer::DiscoveryServer(const QNetworkInterface &iface, quint16 port, bool ip6, QObject *parent)
	: QObject(parent), m_local(false), m_ip6(ip6), m_port(port), m_name(""), m_iface(iface)
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

		qInfo("[discovery] I: Using IP %s (IPv4 mode)", qUtf8Printable(broadcastAddr.toString()));
	}
	else
	{
		QHostAddress multicastAddr;
		multicastAddr.setAddress(QString("ff12::%1").arg(MSG_DISCOVERY_PORT));
		multicastAddr.setScopeId(iface.name());

		m_server->bind(multicastAddr, MSG_DISCOVERY_PORT);
		m_server->joinMulticastGroup(multicastAddr, iface);

		qInfo("[discovery] I: Using interface %s (IPv6 mode)", qUtf8Printable(iface.name()));
	}
}

DiscoveryServer::DiscoveryServer(const QString &name, QObject *parent)
	: QObject(parent), m_local(true), m_ip6(false), m_port(0), m_name(name), m_iface()
{
	m_server = new QUdpSocket(this);
	connect(m_server, &QUdpSocket::readyRead, this, &DiscoveryServer::onReadyRead);

	m_server->bind(QHostAddress::LocalHost, MSG_DISCOVERY_PORT);

	qInfo("[discovery] I: Using IP 127.0.0.1 (IPv4 + local mode)");
}

DiscoveryServer::~DiscoveryServer()
{ }

void DiscoveryServer::onReadyRead()
{
	while(m_server->hasPendingDatagrams())
	{
		QNetworkDatagram datagram = m_server->receiveDatagram();
		QByteArray request = datagram.data();

		if(request.size() != 16)
		{
			continue;
		}

		if(!request.startsWith(MSG_MAGIC_IDENTIFIER))
		{
			continue;
		}

		quint16 messageID = readAsNumber<quint16>(request, 4);
		quint16 messageSize = readAsNumber<quint16>(request, 6);

		if(messageID != MSG_ID_DISCOVERY || messageSize != 8)
		{
			continue;
		}

		request.remove(0, 8);

		QByteArray discoveryResponse, host;

		if(!m_local)
		{
			host = QHostInfo::localHostName().toUtf8();
		}
		else
		{
			host = m_name.toUtf8();
		}

		if(host.length() > 255) host.resize(255);

		discoveryResponse.append(MSG_MAGIC_IDENTIFIER, 4);
		appendAsBytes<quint16>(discoveryResponse, MSG_ID_DISCOVERY);
		appendAsBytes<quint16>(discoveryResponse, 8 + 4 + 16 + 16 + 1 + 1 + 8 + host.length());

		discoveryResponse.append(request);

		appendAsBytes<quint32>(discoveryResponse, ZBNT_VERSION_INT);
		discoveryResponse.append(padString(ZBNT_VERSION_PREREL, 16));
		discoveryResponse.append(padString(ZBNT_VERSION_COMMIT, 16));
		appendAsBytes<quint8>(discoveryResponse, ZBNT_VERSION_DIRTY);

		appendAsBytes<quint8>(discoveryResponse, m_local);

		if(!m_local)
		{
			appendAsBytes<quint64>(discoveryResponse, m_port);
		}
		else
		{
			appendAsBytes<qint64>(discoveryResponse, QCoreApplication::applicationPid());
		}

		discoveryResponse.append(host);

		m_server->writeDatagram(datagram.makeReply(discoveryResponse));
	}
}
