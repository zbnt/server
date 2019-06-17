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

#include <QNetworkInterface>

#include <Utils.hpp>
#include <WorkerThread.hpp>
#include <BitstreamConfig.hpp>

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
	if(m_client)
	{
		QMutexLocker lock(&workerMutex);

		if(msgBuffer.length())
		{
			m_client->write(msgBuffer);
			msgBuffer.clear();
		}
	}
}

void MeasurementServer::onMessageReceived(quint8 id, const QByteArray &data)
{
	QMutexLocker lock(&workerMutex);

	switch(id)
	{
		case MSG_ID_START:
		{
			if(data.size() < 12) return;

			timer->max_time = readAsNumber<uint64_t>(data, 0);
			stats[0]->config = readAsNumber<uint8_t>(data, 8);
			stats[1]->config = readAsNumber<uint8_t>(data, 9);
			stats[2]->config = readAsNumber<uint8_t>(data, 10);
			stats[3]->config = readAsNumber<uint8_t>(data, 11);
			timer->config = 1;

			running = 1;

			for(int i = 0; i < 4; ++i)
			{
				tgenFC[i].paddingPoissonCount = 0;
				tgenFC[i].delayPoissonCount = 0;
			}

			break;
		}

		case MSG_ID_STOP:
		{
			running = 0;
			resetPL();
			break;
		}

		case MSG_ID_TG_CFG:
		{
			if(data.size() < 28) return;

			uint8_t i = readAsNumber<uint8_t>(data, 0);

			if((i >= 2 && bitstream != BITSTREAM_QUAD_TGEN) || i >= 4) return;

			tgen[i]->config = readAsNumber<uint8_t>(data, 1);

			if(tgen[i]->config)
			{
				tgenFC[i].paddingMethod = readAsNumber<uint8_t>(data, 2);

				switch(tgenFC[i].paddingMethod)
				{
					case METHOD_CONSTANT:
					{
						tgen[i]->psize = readAsNumber<uint16_t>(data, 3);
						break;
					}

					case METHOD_RAND_UNIFORM:
					{
						tgenFC[i].paddingBottom = readAsNumber<uint16_t>(data, 5);
						tgenFC[i].paddingTop = readAsNumber<uint16_t>(data, 7) - tgenFC[i].paddingBottom + 1;
						tgen[i]->config |= 1 << 3;
						break;
					}

					case METHOD_RAND_POISSON:
					{
						tgenFC[i].paddingBottom = readAsNumber<uint16_t>(data, 9);
						tgen[i]->config |= 1 << 3;
						break;
					}
				}

				tgenFC[i].delayMethod = readAsNumber<uint8_t>(data, 11);

				switch(tgenFC[i].delayMethod)
				{
					case METHOD_CONSTANT:
					{
						tgen[i]->fdelay = readAsNumber<uint32_t>(data, 12);
						break;
					}

					case METHOD_RAND_UNIFORM:
					{
						tgenFC[i].delayBottom = readAsNumber<uint32_t>(data, 16);
						tgenFC[i].delayTop = readAsNumber<uint32_t>(data, 20) - tgenFC[i].delayBottom + 1;
						tgen[i]->config |= 1 << 2;
						break;
					}

					case METHOD_RAND_POISSON:
					{
						tgenFC[i].delayBottom = readAsNumber<uint32_t>(data, 24);
						tgen[i]->config |= 1 << 2;
						break;
					}
				}
			}

			break;
		}

		case MSG_ID_TG_HEADERS:
		{
			if(data.size() < 15) return;

			uint8_t i = readAsNumber<uint8_t>(data, 0);

			if((i >= 2 && bitstream != BITSTREAM_QUAD_TGEN) || i >= 4) return;

			tgen[i]->hsize = data.length() - 1;
			memcpy_v(tgen[i] + TGEN_MEM_OFFSET, data.constData() + 1, data.length() - 1);

			break;
		}

		case MSG_ID_LM_CFG:
		{
			if(data.size() < 13) return;
			if(bitstream != BITSTREAM_DUAL_TGEN) return;

			measurer->config = readAsNumber<uint8_t>(data, 0);
			measurer->padding = readAsNumber<uint32_t>(data, 1);
			measurer->delay = readAsNumber<uint32_t>(data, 5);
			measurer->timeout = readAsNumber<uint32_t>(data, 9);
			break;
		}
	}
}

void MeasurementServer::onIncomingConnection()
{
	QTcpSocket *connection = m_server->nextPendingConnection();

	if(!m_client)
	{
		m_client = connection;
		m_client->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

		QByteArray helloMsg;
		helloMsg.append(MSG_MAGIC_IDENTIFIER, 4);
		appendAsBytes<quint8>(&helloMsg, MSG_ID_HELLO);
		appendAsBytes<quint32>(&helloMsg, MSG_VERSION);
		m_client->write(helloMsg);

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
	handleIncomingData(m_client->readAll());
}

void MeasurementServer::onNetworkStateChanged(QAbstractSocket::SocketState state)
{
	if(state == QAbstractSocket::UnconnectedState)
	{
		m_client->deleteLater();
		m_client = nullptr;
	}
}
