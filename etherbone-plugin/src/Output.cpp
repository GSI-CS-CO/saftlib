/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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

#include "ECA.hpp"
#include "Output.hpp"
#include "OutputCondition.hpp"
#include "OutputCondition_Service.hpp"

namespace eb_plugin {

// std::shared_ptr<Output> Output::create(const ConstructorType& args)
// {
// 	return RegisteredObject<Output>::create(args.objectPath, args);
// }

Output::Output(ECA &eca
     , Io &io_
     , const std::string &output_object_path
     , const std::string &input_partner_path
     , unsigned channel
     , saftbus::Container *container)
	: ActionSink(eca, output_object_path, io_.getName(), channel, io_.getIndexOut(), container), io(io_), partnerPath(input_partner_path)
{

}

std::string Output::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, bool on)
{
	// the tag parameter is 2 if output should go on 
	//                  and 1 if output should go off
	return NewConditionHelper<OutputCondition>(active, id, mask, offset, on?2:1, container);
}

uint32_t Output::getIndexOut() const
{
	return io.getIndexOut();
}

void Output::WriteOutput(bool value)
{
	ownerOnly();
	return io.WriteOutput(value);
}

bool Output::ReadOutput()
{
	return io.ReadOutput();
}

bool Output::ReadCombinedOutput()
{
	return io.ReadCombinedOutput();
}

bool Output::getOutputEnable() const
{
	return io.getOutputEnable();
}

bool Output::getSpecialPurposeOut() const
{
	return io.getSpecialPurposeOut();
}

bool Output::getGateOut() const
{
	return io.getGateOut();
}

bool Output::getBuTiSMultiplexer() const
{
	return io.getBuTiSMultiplexer();
}

bool Output::getPPSMultiplexer() const
{
	return io.getPPSMultiplexer();
}

bool Output::getOutputEnableAvailable() const
{
	return io.getOutputEnableAvailable();
}

bool Output::getSpecialPurposeOutAvailable() const
{
	return io.getSpecialPurposeOutAvailable();
}

bool Output::StartClock(double high_phase, double low_phase, uint64_t phase_offset)
{
	ownerOnly();
	return io.StartClock(high_phase, low_phase, phase_offset);
}

bool Output::StopClock()
{
	ownerOnly();
	return io.StopClock();
}

std::string Output::getLogicLevelOut() const
{
	return io.getLogicLevelOut();
}

std::string Output::getTypeOut() const
{
	return io.getTypeOut();
}

std::string Output::getInput() const
{
	return partnerPath;
}

void Output::setOutputEnable(bool val)
{
	ownerOnly();
	return io.setOutputEnable(val);
}

void Output::setSpecialPurposeOut(bool val)
{
	ownerOnly();
	return io.setSpecialPurposeOut(val);
}

void Output::setGateOut(bool val)
{
	ownerOnly();
	return io.setGateOut(val);
}

void Output::setBuTiSMultiplexer(bool val)
{
	ownerOnly();
	return io.setBuTiSMultiplexer(val);
}

void Output::setPPSMultiplexer(bool val)
{
	ownerOnly();
	return io.setPPSMultiplexer(val);
}

}
