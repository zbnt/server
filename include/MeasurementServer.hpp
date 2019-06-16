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

class MeasurementServer : public QObject
{
public:
	MeasurementServer(QObject *parent = nullptr);
	~MeasurementServer();

	void sendMeasurements();
	void onMessageReceived(quint8 id, const QByteArray &data);

	void onIncomingConnection();

	void onReadyRead();
	void onNetworkStateChanged(QAbstractSocket::SocketState state);

private:
	QTimer *m_timer = nullptr;
	QTcpServer *m_server = nullptr;
	QTcpSocket *m_client = nullptr;

	RxStatus m_rxStatus = MSG_RX_MAGIC;
	quint16 m_rxByteCount = 0;
	quint16 m_rxMsgSize = 0;
	quint8 m_rxMsgID = 0;
	QByteArray m_rxBuffer;
};
