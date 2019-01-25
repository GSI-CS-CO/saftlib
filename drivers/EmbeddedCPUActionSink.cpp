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

#include "RegisteredObject.h"
#include "EmbeddedCPUActionSink.h"
#include "EmbeddedCPUCondition.h"
#include "TimingReceiver.h"

namespace saftlib {

EmbeddedCPUActionSink::EmbeddedCPUActionSink(const ConstructorType& args)
 : ActionSink(args.objectPath, args.dev, args.name, args.channel, 0), embedded_cpu(args.embedded_cpu)
{
}

const char *EmbeddedCPUActionSink::getInterfaceName() const
{
  return "EmbeddedCPUActionSink";
}

Glib::ustring EmbeddedCPUActionSink::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 tag)
{
  return NewConditionHelper(active, id, mask, offset, tag, false,
    sigc::ptr_fun(&EmbeddedCPUCondition::create));
}

std::shared_ptr<EmbeddedCPUActionSink> EmbeddedCPUActionSink::create(const ConstructorType& args)
{
  return RegisteredObject<EmbeddedCPUActionSink>::create(args.objectPath, args);
}

}
