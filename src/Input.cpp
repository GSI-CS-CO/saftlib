/*  Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <saftbus/error.hpp>

#include "Input.hpp"
#include "eca_tlu_regs.h"

namespace saftlib {

Input::Input( ECA_TLU &tlu
			, const std::string &input_object_path
			, const std::string &output_partner_path
			, unsigned eca_tlu_chan
			, Io *io_
			, saftbus::Container *container)
	: EventSource(input_object_path, io_->getName(), container), eca_tlu(tlu), io(io_), partnerPath(output_partner_path)
	, eca_tlu_channel(eca_tlu_chan), enable(false), event(0), stable(80)
{
	eca_tlu.configInput(eca_tlu_channel, enable, event, stable);
}

uint32_t Input::getIndexIn() const
{
	return io->getIndexIn();
}

bool Input::ReadInput()
{
	return io->ReadInput();
}

bool Input::getInputTermination() const
{
	return io->getInputTermination();
}

bool Input::getSpecialPurposeIn() const
{
	return io->getSpecialPurposeIn();
}

bool Input::getGateIn() const
{
	return io->getGateIn();
}

bool Input::getInputTerminationAvailable() const
{
	return io->getInputTerminationAvailable();
}

bool Input::getSpecialPurposeInAvailable() const
{
	return io->getSpecialPurposeInAvailable();
}

std::string Input::getLogicLevelIn() const
{
	return io->getLogicLevelIn();
}

std::string Input::getTypeIn() const
{
	return io->getTypeIn();
}

std::string Input::getOutput() const
{
	return partnerPath;
}

void Input::setInputTermination(bool val)
{
	ownerOnly();
	return io->setInputTermination(val);
}

void Input::setSpecialPurposeIn(bool val)
{
	ownerOnly();
	return io->setSpecialPurposeIn(val);
}

void Input::setGateIn(bool val)
{
	ownerOnly();
	return io->setGateIn(val);
}

uint64_t Input::getResolution() const
{
	return io->getResolution();
}

// TLU control methods  -------------------------------------------------------------

uint32_t Input::getEventBits() const
{
	return 1; // rising|falling comes from TLU
}

uint32_t Input::getStableTime() const
{
	return stable;
}

bool Input::getEventEnable() const
{
	return enable;
}

uint64_t Input::getEventPrefix() const
{
	return event;
}

void Input::setEventEnable(bool val)
{
	ownerOnly();

	if (enable == val) return;
	enable = val;

	eca_tlu.configInput(eca_tlu_channel, enable, event, stable);
}

void Input::setEventPrefix(uint64_t val)
{
	ownerOnly();

	if (val % 2 != 0)
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "EventPrefix cannot have lowest bit set (EventBits=1)");

	if (event == val) return;
	event = val;

	eca_tlu.configInput(eca_tlu_channel, enable, event, stable);
}

void Input::setStableTime(uint32_t val)
{
	ownerOnly();

	if (val % 8 != 0)
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "StableTime must be a multiple of 8ns");
	if (val < 16)
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "StableTime must be at least 16ns");

	if (stable == val) return;
	stable = val;

	eca_tlu.configInput(eca_tlu_channel, enable, event, stable);
}

}
