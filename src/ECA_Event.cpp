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

#include "ECA_Event.hpp"

#include "eca_regs.h"


namespace saftlib {


#define EVENT_SDB_DEVICE_ID             0x8752bf45


ECA_Event::ECA_Event(etherbone::Device &device, saftbus::Container *container)
	: SdbDevice(device, ECA_SDB_VENDOR_ID, EVENT_SDB_DEVICE_ID)
{
}

void ECA_Event::InjectEventRaw(uint64_t event, uint64_t param, uint64_t time) const
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

