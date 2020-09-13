/*
	zbnt/server
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

#include <AbstractDevice.hpp>
#include <MessageReceiver.hpp>

class ZbntServer : public QObject, public MessageReceiver
{
public:
	ZbntServer(AbstractDevice *parent);
	~ZbntServer();

protected:
	void startRun();
	void stopRun();

	virtual bool clientAvailable() const = 0;
	virtual void sendBytes(const QByteArray &data) = 0;
	virtual void sendBytes(const uint8_t *data, int size) = 0;
	virtual void sendMessage(MessageID id, const QByteArray &data) = 0;

	virtual void onHelloTimeout() = 0;
	void onMessageReceived(quint16 id, const QByteArray &data);

private:
	void handleInterrupt();
	void pollTimer();

protected:
	AbstractDevice *m_device = nullptr;

	QTimer *m_helloTimer = nullptr;
	bool m_helloReceived = false;

private:
	QTimer *m_runEndTimer = nullptr;
	bool m_isRunning = false;

	QByteArray m_pendingDmaData;
	uint32_t m_lastDmaIdx = 0;
};
