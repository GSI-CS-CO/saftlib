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
#include "SCUbusCondition.h"
#include "ActionSink.h"
#include "SAFTd.h"

namespace saftlib {

SCUbusCondition::SCUbusCondition(const ConstructorType& args)
 : Condition(args)
{
}

uint32_t SCUbusCondition::getTag() const
{
  return tag;
}

void SCUbusCondition::setTag(uint32_t val)
{
  ownerOnly();
  if (val == tag) return;
  uint32_t old = tag;
  
  tag = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    tag = old;
    throw;
  }
}

std::shared_ptr<SCUbusCondition> SCUbusCondition::create(const ConstructorType& args)
{
  return RegisteredObject<SCUbusCondition>::create(SAFTd::get().connection(), args.objectPath, args);
}

}
