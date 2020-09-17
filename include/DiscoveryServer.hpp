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

#pragma once

#include <QTimer>
#include <QVector>
#include <QUdpSocket>
#include <QNetworkInterface>

#include <Messages.hpp>
#include <MessageReceiver.hpp>

class DiscoveryServer : public QObject
{
public:
	DiscoveryServer(const QNetworkInterface &iface, quint16 port, bool ip6 = false, QObject *parent = nullptr);
	DiscoveryServer(const QString &name, QObject *parent = nullptr);
	~DiscoveryServer();

	void onReadyRead();

private:
	bool m_local = false;
	bool m_ip6 = false;
	quint16 m_port = 0;
	QString m_name = "";
	QNetworkInterface m_iface;
	QUdpSocket *m_server = nullptr;
};
