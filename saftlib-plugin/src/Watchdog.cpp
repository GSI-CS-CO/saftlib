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

#include "Watchdog.hpp"

#include <saftbus/error.hpp>

#include <iostream>

#define WATCHDOG_VENDOR_ID        0x00000651
#define WATCHDOG_DEVICE_ID        0xb6232cd3

namespace saftlib {

Watchdog::Watchdog(etherbone::Device &dev) 
	: device(dev)
{
	std::vector<sdb_device> watchdogs_dev;
	device.sdb_find_by_identity(WATCHDOG_VENDOR_ID, WATCHDOG_DEVICE_ID, watchdogs_dev);
	if (watchdogs_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no watchdog device found on hardware");
	}
	if (watchdogs_dev.size() > 1) {
		std::cerr << "more than one watchdog device found on hardware, taking the first one" << std::endl;
	}
	watchdog = static_cast<eb_address_t>(watchdogs_dev[0].sdb_component.addr_first);
	if (!aquire()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Watchdog on Timing Receiver already locked");
	}
}

bool Watchdog::aquire() {
	// try to acquire watchdog
	eb_data_t retry;
	device.read(watchdog, EB_DATA32, &watchdog_value);
	if ((watchdog_value & 0xFFFF) != 0) {
		return false;
	}
	device.write(watchdog, EB_DATA32, watchdog_value);
	device.read(watchdog, EB_DATA32, &retry);
	if (((retry ^ watchdog_value) >> 16) != 0) {
		return false;
	}
	return true;
}

void Watchdog::update() {
	device.write(watchdog, EB_DATA32, watchdog_value);
}

} // namespace