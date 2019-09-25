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

#include <WorkerThread.hpp>

#include <Utils.hpp>
#include <Messages.hpp>
#include <BitstreamConfig.hpp>

volatile TrafficGenerator *tgen[4] = {nullptr, nullptr, nullptr, nullptr};
volatile LatencyMeasurer *measurer = nullptr;
volatile StatsCollector *stats[4] = {nullptr, nullptr, nullptr, nullptr};
volatile FrameDetector *detector = nullptr;
volatile SimpleTimer *timer = nullptr;

uint8_t running = 0;
uint8_t streamMode = 0;

QMutex workerMutex;
QByteArray msgBuffer;

double dataRate[4];
uint64_t lastTxBytesCount[4], lastTxBytesTime[4];

void workerThread()
{
	msgBuffer.reserve(2048);

	resetPL();

	while(1)
	{
		QMutexLocker lock(&workerMutex);

		if(!streamMode)
		{
			if(running)
			{
				uint32_t countRead = 0;
				countRead += readFIFO(stats[0], 0);
				countRead += readFIFO(stats[1], 1);
				countRead += readFIFO(stats[2], 2);
				countRead += readFIFO(stats[3], 3);

				switch(bitstream)
				{
					case BITSTREAM_DUAL_TGEN_LATENCY:
					{
						countRead += readFIFO(measurer);
						break;
					}

					case BITSTREAM_DUAL_TGEN_DETECTOR:
					{
						countRead += readFIFO(detector);
						break;
					}
				}

				if(!countRead && !timer->status)
				{
					running = false;
					buildMessage(MSG_ID_DONE, nullptr, 0);
				}
			}
		}
		else
		{
			if(timer->status)
			{
				for(int i = 0; i < 4; ++i)
				{
					stats[i]->config |= (1 << 2);

					if(stats[i]->time != lastTxBytesTime[i])
					{
						dataRate[i] = ((stats[i]->tx_bytes - lastTxBytesCount[i]) / double(stats[i]->time - lastTxBytesTime[i])) * 1000.0;

						lastTxBytesCount[i] = stats[i]->tx_bytes;
						lastTxBytesTime[i] = stats[i]->time;
					}

					stats[i]->config &= ~(1 << 2);
				}
			}
		}
	}
}

void resetPL()
{
	timer->config = 2;

	stats[0]->config = 2;
	stats[1]->config = 2;
	stats[2]->config = 2;
	stats[3]->config = 2;

	tgen[0]->config = 2;
	tgen[1]->config = 2;

	switch(bitstream)
	{
		case BITSTREAM_DUAL_TGEN_LATENCY:
		{
			measurer->config = 2;
			break;
		}

		case BITSTREAM_DUAL_TGEN_DETECTOR:
		{
			detector->config = 2;
			break;
		}

		case BITSTREAM_QUAD_TGEN:
		{
			tgen[2]->config = 2;
			tgen[3]->config = 2;
			break;
		}
	}

	QThread::msleep(100);

	timer->config = 0;
	stats[0]->config = 0;
	stats[1]->config = 0;
	stats[2]->config = 0;
	stats[3]->config = 0;
	tgen[0]->config = 0;
	tgen[1]->config = 0;

	switch(bitstream)
	{
		case BITSTREAM_DUAL_TGEN_LATENCY:
		{
			measurer->config = 0;
			break;
		}

		case BITSTREAM_DUAL_TGEN_DETECTOR:
		{
			detector->config = 0;
			break;
		}

		case BITSTREAM_QUAD_TGEN:
		{
			tgen[2]->config = 0;
			tgen[3]->config = 0;
			break;
		}
	}
}

uint32_t readFIFO(volatile StatsCollector *sc, uint8_t idx)
{
	if(sc->fifo_occupancy)
	{
		sc->fifo_pop = 1;
		buildMessage(MSG_ID_MEASUREMENT_SC, idx, (volatile uint32_t*) &(sc->time), 14);
		return 1;
	}

	return 0;
}

uint32_t readFIFO(volatile LatencyMeasurer *lm)
{
	if(lm->fifo_occupancy)
	{
		lm->fifo_pop = 1;
		buildMessage(MSG_ID_MEASUREMENT_LM, (volatile uint32_t*) &(lm->time), 10);
		return 1;
	}

	return 0;
}

uint32_t readFIFO(volatile FrameDetector *fd)
{
	if(fd->fifo_occupancy)
	{
		fd->fifo_pop = 1;
		buildMessage(MSG_ID_MEASUREMENT_FD, (volatile uint32_t*) &(fd->time_l), 3);
		return 1;
	}

	return 0;
}

void buildMessage(uint8_t id, volatile uint32_t *data, uint32_t numWords)
{
	msgBuffer.append(MSG_MAGIC_IDENTIFIER, 4);
	appendAsBytes<quint8>(&msgBuffer, id);
	appendAsBytes<quint16>(&msgBuffer, numWords * sizeof(uint32_t));

	while(numWords--)
	{
		uint32_t word = *data++;
		appendAsBytes<uint32_t>(&msgBuffer, word);
	}
}

void buildMessage(uint8_t id, uint8_t idx, volatile uint32_t *data, uint32_t numWords)
{
	msgBuffer.append(MSG_MAGIC_IDENTIFIER, 4);
	appendAsBytes<quint8>(&msgBuffer, id);
	appendAsBytes<quint16>(&msgBuffer, numWords * sizeof(uint32_t) + 1);
	appendAsBytes<quint8>(&msgBuffer, idx);

	while(numWords--)
	{
		uint32_t word = *data++;
		appendAsBytes<uint32_t>(&msgBuffer, word);
	}
}
