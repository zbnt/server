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

#include <cstdint>

#include <cores/AbstractCore.hpp>

class FrameDetector : public AbstractCore
{
public:
	static constexpr uint32_t CFG_ENABLE     = 1;
	static constexpr uint32_t CFG_RESET      = 2;
	static constexpr uint32_t CFG_LOG_ENABLE = 4;

	static constexpr uint32_t HAS_CMP_UNIT   = 1;
	static constexpr uint32_t HAS_EDIT_UNIT  = 2;
	static constexpr uint32_t HAS_CSUM_UNIT  = 4;

	struct Registers
	{
		uint16_t config;
		uint16_t log_identifier;
		uint32_t script_enable;
		uint32_t features;
		uint32_t num_scripts;
		uint32_t max_script_size;
		uint32_t script_mem_offset;
		uint32_t tx_fifo_size;
		uint32_t extr_fifo_size;
		uint64_t overflow_count_a;
		uint64_t overflow_count_b;
	};

public:
	FrameDetector(const QString &name, uint32_t id, void *regs, uint8_t portA, uint8_t portB);
	~FrameDetector();

	static AbstractCore *createCore(AbstractDevice *parent, const QString &name, uint32_t id,
	                                void *regs, const void *fdt, int offset);

	void announce(QByteArray &output) const;

	DeviceType getType() const;
	uint64_t getPorts() const;

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, const QByteArray &params, QByteArray &value);

private:
	volatile Registers *m_regs;
	uint8_t m_portA, m_portB;

	QVector<QByteArray> m_scriptNames;
	QVector<uint32_t> m_scriptSizes;
};
