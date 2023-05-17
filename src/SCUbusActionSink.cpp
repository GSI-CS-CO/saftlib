/*  Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
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

#include "SCUbusActionSink.hpp"
#include "ECA.hpp"
#include "eca_queue_regs.h"
#include "eca_flags.h"

#include "Condition.hpp"
#include "SCUbusCondition.hpp"
#include "SCUbusCondition_Service.hpp"


#include <cassert>
#include <sstream>
#include <memory>

namespace saftlib {

SCUbusActionSink::SCUbusActionSink(etherbone::Device &dev
	                              ,ECA &eca
                                  , const std::string &obj_path
                                  , const std::string &name
                                  , unsigned channel, eb_address_t scubus_address
                                  , saftbus::Container *container)
	: ActionSink(eca, obj_path, name, channel, 0, container), device(dev), scubus(scubus_address)
{
}



std::string SCUbusActionSink::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag)
{
	return NewConditionHelper<SCUbusCondition>(active, id, mask, offset, tag, container);
}

void SCUbusActionSink::InjectTag(uint32_t tag)
{
	ownerOnly();
	etherbone::Cycle cycle;
	cycle.open(device);
#define SCUB_SOFTWARE_TAG_LO 0x20
#define SCUB_SOFTWARE_TAG_HI 0x24
	cycle.write(scubus + SCUB_SOFTWARE_TAG_LO, EB_BIG_ENDIAN|EB_DATA16, tag & 0xFFFF);
	cycle.write(scubus + SCUB_SOFTWARE_TAG_HI, EB_BIG_ENDIAN|EB_DATA16, (tag >> 16) & 0xFFFF);
	cycle.close();
}

}
