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

#include <functional>

#include <QByteArray>
#include <QHash>

#include <Messages.hpp>

class AbstractCore;
class AbstractDevice;

using CoreConstructor = std::function<AbstractCore*(AbstractDevice*, const QString&, uint32_t,
                                      void*, const void*, int)>;

using CoreConstructorMap = QHash<QString, CoreConstructor>;

class AbstractCore
{
public:
	AbstractCore(const QString &name, uint32_t id);
	virtual ~AbstractCore();

	static AbstractCore *createCore(AbstractDevice *parent, const QString &compatible,
	                                const QString &name, uint32_t id, void *regs, const void *fdt, int offset);

	virtual void announce(QByteArray &output) const = 0;

	const QString &getName() const;
	virtual DeviceType getType() const = 0;
	virtual uint32_t getIndex() const;
	virtual uint64_t getPorts() const;

	virtual void setReset(bool reset) = 0;
	virtual bool setProperty(PropertyID propID, const QByteArray &value) = 0;
	virtual bool getProperty(PropertyID propID, const QByteArray &params, QByteArray &value) = 0;

protected:
	QString m_name;
	uint32_t m_id;

private:
	static const CoreConstructorMap coreConstructors;
};
