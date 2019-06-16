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

#include <MeasurementServer.hpp>

#include <WorkerThread.hpp>

MeasurementServer::MeasurementServer(QObject *parent) : QObject(parent)
{
	m_timer = new QTimer(this);
	m_timer->setInterval(100);
	m_timer->setSingleShot(false);
	m_timer->start();

	m_server = new QTcpServer(this);
	m_server->listen(QHostAddress::Any, MSG_TCP_PORT);

	connect(m_timer, &QTimer::timeout, this, &MeasurementServer::sendMeasurements);
	connect(m_server, &QTcpServer::newConnection, this, &MeasurementServer::onIncomingConnection);
}

MeasurementServer::~MeasurementServer()
{ }

void MeasurementServer::sendMeasurements()
{
	// TODO: Send messages with measurement results
}

void MeasurementServer::onMessageReceived(quint8 id, const QByteArray &data)
{
	// TODO: Handle message
}

void MeasurementServer::onIncomingConnection()
{
	QTcpSocket *connection = m_server->nextPendingConnection();

	if(!m_client)
	{
		m_client = connection;
		m_client->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

		connect(m_client, &QTcpSocket::readyRead, this, &MeasurementServer::onReadyRead);
		connect(m_client, &QTcpSocket::stateChanged, this, &MeasurementServer::onNetworkStateChanged);
	}
	else
	{
		connection->abort();
		connection->deleteLater();
	}
}

void MeasurementServer::onReadyRead()
{
	QByteArray readData = m_client->readAll();

	for(uint8_t c : readData)
	{
		switch(m_rxStatus)
		{
			case MSG_RX_MAGIC:
			{
				if(c == MSG_MAGIC_IDENTIFIER[m_rxByteCount])
				{
					m_rxByteCount++;

					if(m_rxByteCount == 4)
					{
						m_rxStatus = MSG_RX_HEADER;
						m_rxByteCount = 0;
					}
				}
				else
				{
					m_rxByteCount = 0;
				}

				break;
			}

			case MSG_RX_HEADER:
			{
				switch(m_rxByteCount)
				{
					case 0:
					{
						m_rxMsgID = c;
						m_rxByteCount++;
						break;
					}

					case 1:
					{
						m_rxMsgSize = c;
						m_rxByteCount++;
						break;
					}

					case 2:
					{
						m_rxMsgSize |= c << 8;
						m_rxByteCount = 0;

						if(!m_rxMsgSize)
						{
							m_rxStatus = MSG_RX_MAGIC;
							onMessageReceived(m_rxMsgID, m_rxBuffer);
						}
						else
						{
							m_rxStatus = MSG_RX_DATA;
						}

						break;
					}
				}

				break;
			}

			case MSG_RX_DATA:
			{
				m_rxBuffer.append(c);

				m_rxByteCount++;

				if(m_rxByteCount == m_rxMsgSize)
				{
					m_rxStatus = MSG_RX_MAGIC;
					m_rxByteCount = 0;
					onMessageReceived(m_rxMsgID, m_rxBuffer);
					m_rxBuffer.clear();
				}

				break;
			}
		}
	}
}

void MeasurementServer::onNetworkStateChanged(QAbstractSocket::SocketState state)
{
	if(state == QAbstractSocket::UnconnectedState)
	{
		m_client->deleteLater();
		m_client = nullptr;
	}
}
