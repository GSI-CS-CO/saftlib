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

#include "Input.h"
#include "RegisteredObject.h"
#include "eca_tlu_regs.h"

namespace saftlib {

std::shared_ptr<Input> Input::create(const ConstructorType& args)
{
  return RegisteredObject<Input>::create(args.objectPath, args);
}

Input::Input(const ConstructorType& args)
 : EventSource(args.objectPath, args.dev, args.name, args.destroy),
   impl(args.impl), partnerPath(args.partnerPath), 
   tlu(args.tlu), channel(args.channel), enable(false), event(0), stable(80)
{
  impl->InputTermination.connect(InputTermination.make_slot());
  impl->SpecialPurposeIn.connect(SpecialPurposeIn.make_slot());
  // initialize TLU to disabled state
  configInput();
}

const char *Input::getInterfaceName() const
{
  return "Input";
}

// Proxy methods to the InoutImpl --------------------------------------------------

bool Input::ReadInput()
{
  return impl->ReadInput();
}

bool Input::getInputTermination() const
{
  return impl->getInputTermination();
}

bool Input::getSpecialPurposeIn() const
{
  return impl->getSpecialPurposeIn();
}

bool Input::getInputTerminationAvailable() const
{
  return impl->getInputTerminationAvailable();
}

bool Input::getSpecialPurposeInAvailable() const
{
  return impl->getSpecialPurposeInAvailable();
}

std::string Input::getLogicLevelIn() const
{
  return impl->getLogicLevelIn();
}

std::string Input::getTypeIn() const
{
  return impl->getTypeIn();
}

std::string Input::getOutput() const
{
  return partnerPath;
}

void Input::setInputTermination(bool val)
{
  ownerOnly();
  return impl->setInputTermination(val);
}

void Input::setSpecialPurposeIn(bool val)
{
  ownerOnly();
  return impl->setSpecialPurposeIn(val);
}

uint64_t Input::getResolution() const
{
  return impl->getResolution();
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
  
  configInput();
  EventEnable(val);
}

void Input::setEventPrefix(uint64_t val)
{
  ownerOnly();
  
  if (val % 2 != 0)
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "EventPrefix cannot have lowest bit set (EventBits=1)");
  
  if (event == val) return;
  event = val;
  
  configInput();
  EventPrefix(val);
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
  
  configInput();
  StableTime(val);
}

void Input::configInput()
{
  etherbone::Cycle cycle;
  cycle.open(dev->getDevice());
  cycle.write(tlu + ECA_TLU_INPUT_SELECT_RW, EB_DATA32, channel);
  cycle.write(tlu + ECA_TLU_ENABLE_RW,       EB_DATA32, enable?1:0);
  cycle.write(tlu + ECA_TLU_STABLE_RW,       EB_DATA32, stable/8 - 1);
  cycle.write(tlu + ECA_TLU_EVENT_HI_RW,     EB_DATA32, event >> 32);
  cycle.write(tlu + ECA_TLU_EVENT_LO_RW,     EB_DATA32, (uint32_t)event);
  cycle.write(tlu + ECA_TLU_WRITE_OWR,       EB_DATA32, 1);
  cycle.close();
}

}
