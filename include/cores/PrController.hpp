/*
	zbnt/server
	Copyright (C) 2020 Oscar R.

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

#include <AbstractDevice.hpp>
#include <cores/AbstractCore.hpp>

class PrController : public AbstractCore
{
public:
	static constexpr uint32_t CMD_SHUTDOWN       = 0b000;
	static constexpr uint32_t CMD_RESTART        = 0b001;
	static constexpr uint32_t CMD_RESTART_STATUS = 0b010;
	static constexpr uint32_t CMD_PROCEED        = 0b011;
	static constexpr uint32_t CMD_USER_CONTROL   = 0b100;

	static constexpr uint32_t ST_SHUTDOWN        = 1u << 7;
	static constexpr uint32_t ST_LOADING         = 0b100;
	static constexpr uint32_t ST_ACTIVE_OKAY     = 0b111;

	static constexpr uint32_t TRIGGER_PENDING    = 1u << 31;

	struct Registers
	{
		uint32_t status;
		uint32_t trigger;
	};

public:
	PrController(const QString &name, uint32_t id, void *regs, const BitstreamList &bitstreams);
	~PrController();

	static AbstractCore *createCore(AbstractDevice *parent, const QString &name, uint32_t id,
	                                void *regs, const void *fdt, int offset);

	void announce(QByteArray &output) const;

	DeviceType getType() const;

	uint32_t status() const;
	bool loadBitstream(const QString &name);
	int activeBitstream() const;
	const BitstreamList bitstreamList() const;

	void setReset(bool reset);
	bool setProperty(PropertyID propID, const QByteArray &value);
	bool getProperty(PropertyID propID, const QByteArray &params, QByteArray &value);

private:
	volatile Registers *m_regs;
	BitstreamList m_bitstreamNames;
};

