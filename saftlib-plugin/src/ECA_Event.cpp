/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#include "ECA_Event.hpp"
// #include "SAFTd.hpp"

// #include <stdlib.h>
// #include <string.h>
// #include <iostream>
// #include <sstream>
// #include <iomanip>
// #include <memory>
// #include <algorithm>
// #include <cassert>

// #include <saftbus/error.hpp>
// #include <saftbus/service.hpp>

// #include "SoftwareActionSink.hpp"
// #include "SoftwareActionSink_Service.hpp"
// #include "SCUbusActionSink.hpp"
// #include "SCUbusActionSink_Service.hpp"
// #include "EmbeddedCPUActionSink.hpp"
// #include "EmbeddedCPUActionSink_Service.hpp"
// #include "WbmActionSink.hpp"
// #include "WbmActionSink_Service.hpp"
// #include "Output.hpp"

#include "eca_regs.h"
// #include "eca_flags.h"
// #include "eca_queue_regs.h"
// #include "fg_regs.h"
// #include "ats_regs.h"


namespace saftlib {


#define EVENT_SDB_DEVICE_ID             0x8752bf45





ECA_Event::ECA_Event(etherbone::Device &device, saftbus::Container *container)
	: SdbDevice(device, ECA_SDB_VENDOR_ID, EVENT_SDB_DEVICE_ID)
{
}

void ECA_Event::InjectEventRaw(uint64_t event, uint64_t param, uint64_t time)
{
	etherbone::Cycle cycle;

	cycle.open(device);
	cycle.write(adr_first, EB_DATA32, event >> 32);
	cycle.write(adr_first, EB_DATA32, event & 0xFFFFFFFFUL);
	cycle.write(adr_first, EB_DATA32, param >> 32);
	cycle.write(adr_first, EB_DATA32, param & 0xFFFFFFFFUL);
	cycle.write(adr_first, EB_DATA32, 0); // reserved
	cycle.write(adr_first, EB_DATA32, 0); // TEF
	cycle.write(adr_first, EB_DATA32, time >> 32);
	cycle.write(adr_first, EB_DATA32, time & 0xFFFFFFFFUL);
	cycle.close();
}




} // namespace

