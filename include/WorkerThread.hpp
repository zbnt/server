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

#include <QMutex>
#include <QThread>
#include <QMutexLocker>
#include <QByteArray>

#include <TrafficGenerator.hpp>
#include <LatencyMeasurer.hpp>
#include <StatsCollector.hpp>
#include <FrameDetector.hpp>
#include <SimpleTimer.hpp>

enum
{
	METHOD_CONSTANT,
	METHOD_RAND_UNIFORM,
	METHOD_RAND_POISSON
};

struct TGenFifoConfig
{
	uint8_t delayMethod;
	uint8_t paddingMethod;

	uint16_t paddingBottom;
	uint16_t paddingTop;

	uint32_t delayBottom;
	uint32_t delayTop;

	uint64_t paddingPoissonCount;
	uint16_t delayPoissonCount;
};

extern volatile uint32_t *temac[4];
extern volatile TrafficGenerator *tgen[4];
extern volatile LatencyMeasurer *measurer;
extern volatile StatsCollector *stats[4];
extern volatile FrameDetector *detector;
extern volatile SimpleTimer *timer;

extern QMutex workerMutex;
extern QByteArray msgBuffer;

extern uint8_t running;
extern uint8_t streamMode;
extern TGenFifoConfig tgenFC[4];

extern double dataRate[4];
extern uint64_t lastTxBytesCount[4], lastTxBytesTime[4];

extern void workerThread();
extern void resetPL();
extern void fillFIFO();
extern void writeFIFO(volatile TrafficGenerator *tg, TGenFifoConfig *fcfg);
extern uint32_t readFIFO(volatile StatsCollector *sc, uint8_t idx);
extern uint32_t readFIFO(volatile LatencyMeasurer *lm);
extern uint32_t readFIFO(volatile FrameDetector *fd);
extern void buildMessage(uint8_t id, volatile uint32_t *data, uint32_t numWords);
extern void buildMessage(uint8_t id, uint8_t idx, volatile uint32_t *data, uint32_t numWords);
