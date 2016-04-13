/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#include "Input.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Input> Input::create(const ConstructorType& args)
{
  return RegisteredObject<Input>::create(args.objectPath, args);
}

Input::Input(const ConstructorType& args)
 : EventSource(args.objectPath, args.dev, args.name, args.destroy),
   impl(args.impl), partnerPath(args.partnerPath)
{
  impl->StableTime.connect(StableTime.make_slot());
  impl->InputTermination.connect(InputTermination.make_slot());
  impl->SpecialPurposeIn.connect(SpecialPurposeIn.make_slot());
}

const char *Input::getInterfaceName() const
{
  return "Input";
}

bool Input::ReadInput()
{
  return impl->ReadInput();
}

guint32 Input::getStableTime() const 
{
  return impl->getStableTime();
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

Glib::ustring Input::getLogicLevelIn() const
{
  return impl->getLogicLevelIn();
}

Glib::ustring Input::getOutput() const
{
  return partnerPath;
}

void Input::setStableTime(guint32 val)
{
  ownerOnly();
  return impl->setStableTime(val);
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

}
