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

#include <cores/AbstractCore.hpp>

#include <AbstractDevice.hpp>
#include <cores/AxiDma.hpp>
#include <cores/AxiMdio.hpp>
#include <cores/FrameDetector.hpp>
#include <cores/LatencyMeasurer.hpp>
#include <cores/SimpleTimer.hpp>
#include <cores/StatsCollector.hpp>
#include <cores/TrafficGenerator.hpp>

const CoreConstructorMap AbstractCore::coreConstructors =
{
	{"zbnt,message-dma",       AxiDma::createCore},
	{"zbnt,axi-mdio",          AxiMdio::createCore},
	{"zbnt,frame-detector",    FrameDetector::createCore},
	{"zbnt,latency-measurer",  LatencyMeasurer::createCore},
	{"zbnt,simple-timer",      SimpleTimer::createCore},
	{"zbnt,stats-collector",   StatsCollector::createCore},
	{"zbnt,traffic-generator", TrafficGenerator::createCore}
};

AbstractCore::AbstractCore(const QString &name, uint32_t id)
	: m_name(name), m_id(id)
{ }

AbstractCore::~AbstractCore()
{ }

AbstractCore *AbstractCore::createCore(AbstractDevice *parent, const QString &compatible,
                                       const QString &name, uint32_t id, void *regs, const void *fdt, int offset)
{
	auto it = coreConstructors.constFind(compatible);

	if(it != coreConstructors.constEnd())
	{
		return (*it)(parent, name, id, regs, fdt, offset);
	}

	return nullptr;
}

const QString &AbstractCore::getName() const
{
	return m_name;
}

uint32_t AbstractCore::getIndex() const
{
	return m_id;
}

uint64_t AbstractCore::getPorts() const
{
	return 0;
}
