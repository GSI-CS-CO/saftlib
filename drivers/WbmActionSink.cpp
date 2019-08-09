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
#include "WbmActionSink.h"
#include "WbmCondition.h"
#include "TimingReceiver.h"

namespace saftlib {

WbmActionSink::WbmActionSink(const ConstructorType& args)
 : ActionSink(args.objectPath, args.dev, args.name, args.channel, 0), scubus(args.scubus)
{
}

const char *WbmActionSink::getInterfaceName() const
{
  return "WbmActionSink";
}

std::string WbmActionSink::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag)
{
  return NewConditionHelper(active, id, mask, offset, tag, false,
    sigc::ptr_fun(&WbmCondition::create));
}

// void WbmActionSink::InjectTag(uint32_t tag)
// {
//   ownerOnly();
//   etherbone::Cycle cycle;
//   cycle.open(dev->getDevice());
// #define SCUB_SOFTWARE_TAG_LO 0x20
// #define SCUB_SOFTWARE_TAG_HI 0x24
//   cycle.write(scubus + SCUB_SOFTWARE_TAG_LO, EB_BIG_ENDIAN|EB_DATA16, tag & 0xFFFF);
//   cycle.write(scubus + SCUB_SOFTWARE_TAG_HI, EB_BIG_ENDIAN|EB_DATA16, (tag >> 16) & 0xFFFF);
//   cycle.close();
// }

std::shared_ptr<WbmActionSink> WbmActionSink::create(const ConstructorType& args)
{
  return RegisteredObject<WbmActionSink>::create(args.objectPath, args);
}

}
