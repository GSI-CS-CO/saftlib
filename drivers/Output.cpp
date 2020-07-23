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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include "Output.h"
#include "OutputCondition.h"
#include "RegisteredObject.h"
#include "SAFTd.h"

namespace saftlib {

std::shared_ptr<Output> Output::create(const ConstructorType& args)
{
  return RegisteredObject<Output>::create(SAFTd::get().connection(), args.objectPath, args);
}

Output::Output(const ConstructorType& args) :
  ActionSink(args.objectPath, args.dev, args.name, args.channel, args.num, args.destroy),
  impl(args.impl), partnerPath(args.partnerPath)
{
}

const char *Output::getInterfaceName() const
{
  return "Output";
}

std::string Output::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, bool on)
{
  return NewConditionHelper(active, id, mask, offset, on?2:1, false, // 2 is on, 1 is off
    sigc::ptr_fun(&OutputCondition::create));
}

uint32_t Output::getIndexOut() const
{
  return impl->getIndexOut();
}

void Output::WriteOutput(bool value)
{
  ownerOnly();
  return impl->WriteOutput(value);
}

bool Output::ReadOutput()
{
  return impl->ReadOutput();
}

bool Output::getOutputEnable() const
{
  return impl->getOutputEnable();
}

bool Output::getSpecialPurposeOut() const
{
  return impl->getSpecialPurposeOut();
}

bool Output::getGateOut() const
{
  return impl->getGateOut();
}

bool Output::getBuTiSMultiplexer() const
{
  return impl->getBuTiSMultiplexer();
}

bool Output::getPPSMultiplexer() const
{
  return impl->getPPSMultiplexer();
}

bool Output::getOutputEnableAvailable() const
{
  return impl->getOutputEnableAvailable();
}

bool Output::getSpecialPurposeOutAvailable() const
{
  return impl->getSpecialPurposeOutAvailable();
}

bool Output::StartClock(double high_phase, double low_phase, uint64_t phase_offset)
{
  ownerOnly();
  return impl->StartClock(high_phase, low_phase, phase_offset);
}

bool Output::StopClock()
{
  ownerOnly();
  return impl->StopClock();
}

std::string Output::getLogicLevelOut() const
{
  return impl->getLogicLevelOut();
}

std::string Output::getTypeOut() const
{
  return impl->getTypeOut();
}

std::string Output::getInput() const
{
  return partnerPath;
}

void Output::setOutputEnable(bool val)
{
  ownerOnly();
  return impl->setOutputEnable(val);
}

void Output::setSpecialPurposeOut(bool val)
{
  ownerOnly();
  return impl->setSpecialPurposeOut(val);
}

void Output::setGateOut(bool val)
{
  ownerOnly();
  return impl->setGateOut(val);
}

void Output::setBuTiSMultiplexer(bool val)
{
  ownerOnly();
  return impl->setBuTiSMultiplexer(val);
}

void Output::setPPSMultiplexer(bool val)
{
  ownerOnly();
  return impl->setPPSMultiplexer(val);
}

}
