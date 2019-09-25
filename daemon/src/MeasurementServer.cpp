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

	m_streamServer = new QTcpServer(this);
	m_streamServer->listen(QHostAddress::Any, MSG_TCP_STREAM_PORT);

	connect(m_timer, &QTimer::timeout, this, &MeasurementServer::sendMeasurements);
	connect(m_server, &QTcpServer::newConnection, this, &MeasurementServer::onIncomingConnection);
	connect(m_streamServer, &QTcpServer::newConnection, this, &MeasurementServer::onIncomingStreamConnection);
}

MeasurementServer::~MeasurementServer()
{ }

void MeasurementServer::sendMeasurements()
{
	if(!streamMode)
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
	else
	{
		if(m_streamClient)
		{
			QMutexLocker lock(&workerMutex);
			QByteArray streamData;

			appendAsBytes<double>(&streamData, dataRate[0]);
			appendAsBytes<double>(&streamData, dataRate[1]);
			appendAsBytes<double>(&streamData, dataRate[2]);
			appendAsBytes<double>(&streamData, dataRate[3]);

			m_streamClient->write(streamData);
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
			stats[0]->config = readAsNumber<uint8_t>(data, 8) | (1 << 3);
			stats[1]->config = readAsNumber<uint8_t>(data, 9) | (1 << 3);
			stats[2]->config = readAsNumber<uint8_t>(data, 10) | (1 << 3);
			stats[3]->config = readAsNumber<uint8_t>(data, 11) | (1 << 3);

			timer->config = 1;
			running = 1;

			if(m_streamClient)
			{
				m_streamClient->abort();
			}

			streamMode = 0;
			m_timer->setInterval(100);

			break;
		}

		case MSG_ID_START_STREAM:
		{
			if(data.size() < 2) return;

			timer->max_time = 0xFFFFFFFF'FFFFFFFFull;
			timer->config = 1;

			running = 1;
			streamMode = 1;

			for(int i = 0; i < 4; ++i)
			{
				stats[i]->config = 1;

				dataRate[i] = 0;
				lastTxBytesCount[i] = 0;
			}

			m_timer->setInterval(readAsNumber<uint16_t>(data, 0));

			break;
		}

		case MSG_ID_STOP:
		{
			if(m_streamClient)
			{
				m_streamClient->abort();
			}

			streamMode = 0;
			m_timer->setInterval(100);

			running = 0;
			resetPL();
			break;
		}

		case MSG_ID_SET_BITSTREAM:
		{
			if(data.size() < 1) return;

			BitstreamID bid = (BitstreamID) readAsNumber<uint8_t>(data, 0);

			running = 0;

			if(bid != bitstream)
			{
				programPL(bid);
				QThread::sleep(5);
				resetPL();
			}

			break;
		}

		case MSG_ID_TG_CFG:
		{
			if(data.size() < 21) return;

			uint8_t i = readAsNumber<uint8_t>(data, 0);

			if((i >= 2 && bitstream != BITSTREAM_QUAD_TGEN) || i >= 4) return;

			tgen[i]->config = readAsNumber<uint8_t>(data, 1);
			tgen[i]->fsize = readAsNumber<uint16_t>(data, 2);
			tgen[i]->fdelay = readAsNumber<uint32_t>(data, 4);
			tgen[i]->lfsr_seed_val = readAsNumber<uint64_t>(data, 13);
			tgen[i]->lfsr_seed_req = 1;

			uint8_t useBurst = readAsNumber<uint8_t>(data, 8);

			if(useBurst)
			{
				tgen[i]->burst_time_on = readAsNumber<uint16_t>(data, 9);
				tgen[i]->burst_time_off = readAsNumber<uint16_t>(data, 11);
				tgen[i]->config |= 1 << 4;
			}

			break;
		}

		case MSG_ID_TG_FRAME:
		{
			if(data.size() < 15 || data.size() > 2049) return;

			uint8_t i = readAsNumber<uint8_t>(data, 0);

			if((i >= 2 && bitstream != BITSTREAM_QUAD_TGEN) || i >= 4) return;

			memcpy_v((volatile uint8_t*) tgen[i] + TGEN_MEM_FRAME_OFFSET, data.constData() + 1, data.length() - 1);

			break;
		}

		case MSG_ID_TG_PATTERN:
		{
			if(data.size() != 257) return;

			uint8_t i = readAsNumber<uint8_t>(data, 0);

			if((i >= 2 && bitstream != BITSTREAM_QUAD_TGEN) || i >= 4) return;

			memcpy_v((volatile uint8_t*) tgen[i] + TGEN_MEM_PATTERN_OFFSET, data.constData() + 1, data.length() - 1);

			break;
		}

		case MSG_ID_LM_CFG:
		{
			if(data.size() < 13) return;
			if(bitstream != BITSTREAM_DUAL_TGEN_LATENCY) return;

			measurer->config = readAsNumber<uint8_t>(data, 0);
			measurer->padding = readAsNumber<uint32_t>(data, 1);
			measurer->delay = readAsNumber<uint32_t>(data, 5);
			measurer->timeout = readAsNumber<uint32_t>(data, 9);
			break;
		}

		case MSG_ID_FD_CFG:
		{
			if(data.size() < 2) return;
			if(bitstream != BITSTREAM_DUAL_TGEN_DETECTOR) return;

			detector->config = (readAsNumber<uint8_t>(data, 0) & 0b1) | ((readAsNumber<uint8_t>(data, 1) & 0b111111) << 2);
			break;
		}

		case MSG_ID_FD_PATTERNS:
		{
			if(data.size() < FD_MEM_SIZE + 1) return;
			if(bitstream != BITSTREAM_DUAL_TGEN_DETECTOR) return;

			uint8_t i = readAsNumber<uint8_t>(data, 0);

			if(!i)
			{
				memcpy_v((volatile uint8_t*) detector + FD_MEM_A_OFFSET, data.constData() + 1, FD_MEM_SIZE);
			}
			else
			{
				memcpy_v((volatile uint8_t*) detector + FD_MEM_B_OFFSET, data.constData() + 1, FD_MEM_SIZE);
			}

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

		connect(m_client, &QTcpSocket::readyRead, this, &MeasurementServer::onReadyRead);
		connect(m_client, &QTcpSocket::stateChanged, this, &MeasurementServer::onNetworkStateChanged);
	}
	else
	{
		connection->abort();
		connection->deleteLater();
	}
}

void MeasurementServer::onIncomingStreamConnection()
{
	QTcpSocket *connection = m_streamServer->nextPendingConnection();

	if(!m_streamClient && streamMode)
	{
		m_streamClient = connection;
		m_streamReadBuffer.clear();

		connect(m_streamClient, &QTcpSocket::readyRead, this, &MeasurementServer::onStreamReadyRead);
		connect(m_streamClient, &QTcpSocket::stateChanged, this, &MeasurementServer::onStreamNetworkStateChanged);
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

void MeasurementServer::onStreamReadyRead()
{
	m_streamReadBuffer += m_streamClient->readAll();

	// Frame format:
	//    [ EN0 | EN1 | EN2 | EN3 | PSIZE0 | PSIZE1 | PSIZE2 | PSIZE3 | FDELAY0 | FDELAY1 | FDELAY2 | FDELAY3 ]
	//       4     4     4     4      4        4        4        4         4         4         4         4
	//       = 48 bytes

	if(m_streamReadBuffer.length() >= 48*2)
	{
		m_streamReadBuffer.remove(0, m_streamReadBuffer.length() - 48 - m_streamReadBuffer.length() % 48);
	}

	if(m_streamReadBuffer.length() >= 48)
	{
		QMutexLocker lock(&workerMutex);

		tgen[0]->config = readAsNumber<uint8_t>(m_streamReadBuffer, 0);
		tgen[1]->config = readAsNumber<uint8_t>(m_streamReadBuffer, 4);

		tgen[0]->fsize = readAsNumber<uint16_t>(m_streamReadBuffer, 16);
		tgen[1]->fsize = readAsNumber<uint16_t>(m_streamReadBuffer, 20);

		tgen[0]->fdelay = readAsNumber<uint32_t>(m_streamReadBuffer, 32);
		tgen[1]->fdelay = readAsNumber<uint32_t>(m_streamReadBuffer, 36);

		if(bitstream == BITSTREAM_QUAD_TGEN)
		{
			tgen[2]->config = readAsNumber<uint8_t>(m_streamReadBuffer, 8);
			tgen[3]->config = readAsNumber<uint8_t>(m_streamReadBuffer, 12);

			tgen[2]->fsize = readAsNumber<uint16_t>(m_streamReadBuffer, 24);
			tgen[3]->fsize = readAsNumber<uint16_t>(m_streamReadBuffer, 28);

			tgen[2]->fdelay = readAsNumber<uint32_t>(m_streamReadBuffer, 40);
			tgen[3]->fdelay = readAsNumber<uint32_t>(m_streamReadBuffer, 44);
		}
	}
}

void MeasurementServer::onStreamNetworkStateChanged(QAbstractSocket::SocketState state)
{
	if(state == QAbstractSocket::UnconnectedState)
	{
		m_streamClient->deleteLater();
		m_streamClient = nullptr;
	}
}
