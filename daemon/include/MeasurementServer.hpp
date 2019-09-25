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
#include <QTcpServer>
#include <QTcpSocket>

#include <Messages.hpp>
#include <MessageReceiver.hpp>

class MeasurementServer : public QObject, public MessageReceiver
{
public:
	MeasurementServer(QObject *parent = nullptr);
	~MeasurementServer();

	void sendMeasurements();
	void onMessageReceived(quint8 id, const QByteArray &data);

	void onIncomingConnection();
	void onIncomingStreamConnection();

	void onReadyRead();
	void onNetworkStateChanged(QAbstractSocket::SocketState state);

	void onStreamReadyRead();
	void onStreamNetworkStateChanged(QAbstractSocket::SocketState state);

private:
	QTimer *m_timer = nullptr;
	QTcpServer *m_server = nullptr, *m_streamServer = nullptr;
	QTcpSocket *m_client = nullptr, *m_streamClient = nullptr;

	QByteArray m_streamReadBuffer;
};