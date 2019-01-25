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

#include "RegisteredObject.h"
#include "OutputCondition.h"
#include "ActionSink.h"

namespace saftlib {

OutputCondition::OutputCondition(const ConstructorType& args)
 : Condition(args)
{
}

bool OutputCondition::getOn() const
{
  return tag == 2;
}

void OutputCondition::setOn(bool v)
{
  guint32 val = v?2:1; // 2 = turn-on, 1 = turn-off
  
  ownerOnly();
  if (val == tag) return;
  guint32 old = tag;
  
  tag = val;
  try {
    if (active) sink->compile();
    On(tag);
  } catch (...) {
    tag = old;
    throw;
  }
}

std::shared_ptr<OutputCondition> OutputCondition::create(const ConstructorType& args)
{
  return RegisteredObject<OutputCondition>::create(args.objectPath, args);
}

}
