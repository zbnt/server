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

	void startRun();
	void stopRun();

	void pollAxiTimer();
	void flushDmaBuffer();
	void onHelloTimeout();
	void onMessageReceived(quint16 id, const QByteArray &data);

	void onIncomingConnection();
	void onReadyRead();
	void onNetworkStateChanged(QAbstractSocket::SocketState state);

private:
	QTimer *m_dmaTimer = nullptr;
	QByteArray m_pendingDmaData;
	uint32_t m_lastDmaIdx = 0;

	QTimer *m_helloTimer = nullptr;
	bool m_helloReceived = false;

	QTimer *m_runEndTimer = nullptr;

	QTcpServer *m_server = nullptr;
	QTcpSocket *m_client = nullptr;
};

extern MeasurementServer *g_measurementSrv;
