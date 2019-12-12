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

extern QMutex workerMutex;
extern QByteArray msgBuffer;

extern uint8_t running;
extern uint8_t streamMode;

extern double dataRate[4];
extern uint64_t lastTxBytesCount[4], lastTxBytesTime[4];

extern void workerThread();
extern void resetPL();
extern void buildMessage(uint8_t id, volatile uint32_t *data, uint32_t numWords);
extern void buildMessage(uint8_t id, uint8_t idx, volatile uint32_t *data, uint32_t numWords);
