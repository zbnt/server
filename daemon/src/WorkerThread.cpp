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

#include <BitstreamManager.hpp>

QMutex g_workerMutex;
QByteArray g_msgBuffer;

void workerThread()
{
	g_msgBuffer.reserve(1024 * 1024);

	while(1)
	{
		if(g_axiDma && g_axiDma->isReady())
		{
			g_axiDma->waitForInterrupt();
			g_workerMutex.lock();

			g_dmaBuffer->copyBuffer(g_msgBuffer);

			g_workerMutex.unlock();
			g_axiDma->clearInterrupts();
			g_axiDma->startTransfer();
		}
	}
}
