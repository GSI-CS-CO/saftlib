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
#include "MILbusActionSink.h"
#include "MILbusCondition.h"

namespace saftlib {

MILbusActionSink::MILbusActionSink(const ConstructorType& args)
 : ActionSink(args.objectPath, args.dev, args.name, args.channel, 0)
{
}

const char *MILbusActionSink::getInterfaceName() const
{
  return "MILbusActionSink";
}

std::string MILbusActionSink::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint16_t tag)
{
  return NewConditionHelper(active, id, mask, offset, tag, false,
    sigc::ptr_fun(&MILbusCondition::create));
}

void MILbusActionSink::InjectTag(uint16_t tag)
{
  ownerOnly();
  throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

std::shared_ptr<MILbusActionSink> MILbusActionSink::create(const ConstructorType& args)
{
  return RegisteredObject<MILbusActionSink>::create(args.objectPath, args);
}

}
