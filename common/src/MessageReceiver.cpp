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

#include <MessageReceiver.hpp>

MessageReceiver::MessageReceiver()
{ }

MessageReceiver::~MessageReceiver()
{ }

void MessageReceiver::handleIncomingData(const QByteArray &readData)
{
	for(quint8 c : readData)
	{
		switch(m_rxStatus)
		{
			case MSG_RX_MAGIC:
			{
				if(c == quint8(MSG_MAGIC_IDENTIFIER[m_rxByteCount]))
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
						m_rxMsgID |= c << 8;
						m_rxByteCount++;
						break;
					}

					case 2:
					{
						m_rxMsgSize = c;
						m_rxByteCount++;
						break;
					}

					case 3:
					{
						m_rxMsgSize |= c << 8;
						m_rxByteCount = 0;

						if(m_rxMsgID == MSG_ID_EXTENDED)
						{
							m_rxStatus = MSG_RX_EXTENDED_HEADER;
							m_rxMsgID = m_rxMsgSize;
						}
						else
						{
							if(!m_rxMsgSize)
							{
								m_rxStatus = MSG_RX_MAGIC;
								onMessageReceived(m_rxMsgID, m_rxBuffer);
							}
							else
							{
								m_rxStatus = MSG_RX_DATA;
							}
						}

						break;
					}
				}

				break;
			}

			case MSG_RX_EXTENDED_HEADER:
			{
				switch(m_rxByteCount)
				{
					case 0:
					{
						m_rxMsgSize = c;
						m_rxByteCount++;
						break;
					}

					case 1:
					{
						m_rxMsgSize |= c << 8;
						m_rxByteCount++;
						break;
					}

					case 2:
					{
						m_rxMsgSize |= c << 16;
						m_rxByteCount++;
						break;
					}

					case 3:
					{
						m_rxMsgSize |= c << 24;
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
