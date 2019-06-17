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

#pragma once

#include <QTimer>
#include <QUdpSocket>

#include <Messages.hpp>
#include <MessageReceiver.hpp>

class DiscoveryServer : public QObject, public MessageReceiver
{
public:
	DiscoveryServer(QObject *parent = nullptr);
	~DiscoveryServer();

	void onMessageReceived(quint8 id, const QByteArray &data);
	void onReadyRead();

private:
	bool m_received = false;
	QUdpSocket *m_server = nullptr;
};

