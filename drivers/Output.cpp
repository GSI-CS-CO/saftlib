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

namespace saftlib {

Glib::RefPtr<Output> Output::create(const ConstructorType& args)
{
  return RegisteredObject<Output>::create(args.objectPath, args);
}

Output::Output(const ConstructorType& args) : 
  ActionSink(args.objectPath, args.dev, args.name, args.channel, args.num, args.destroy),
  impl(args.impl), partnerPath(args.partnerPath)
{
  impl->OutputEnable.connect(OutputEnable.make_slot());
  impl->SpecialPurposeOut.connect(SpecialPurposeOut.make_slot());
  impl->BuTiSMultiplexer.connect(BuTiSMultiplexer.make_slot());
}

const char *Output::getInterfaceName() const
{
  return "Output";
}

Glib::ustring Output::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, bool on)
{
  return NewConditionHelper(active, id, mask, offset, on?2:1, false, // 2 is on, 1 is off
    sigc::ptr_fun(&OutputCondition::create));
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

bool Output::getBuTiSMultiplexer() const
{
  return impl->getBuTiSMultiplexer();
}

bool Output::getOutputEnableAvailable() const
{
  return impl->getOutputEnableAvailable();
}

bool Output::getSpecialPurposeOutAvailable() const
{
  return impl->getSpecialPurposeOutAvailable();
}

Glib::ustring Output::getLogicLevelOut() const
{
  return impl->getLogicLevelOut();
}

Glib::ustring Output::getTypeOut() const
{
  return impl->getTypeOut();
}

Glib::ustring Output::getInput() const
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

void Output::setBuTiSMultiplexer(bool val)
{
  ownerOnly();
  return impl->setBuTiSMultiplexer(val);
}

}
