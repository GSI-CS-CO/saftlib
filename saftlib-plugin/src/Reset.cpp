/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
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

#include "Reset.hpp"

#include <saftbus/error.hpp>

#include <iostream>

#define RESET_VENDOR_ID         0x651
#define RESET_DEVICE_ID         0x3a362063

#define FPGA_RESET_WATCHDOG_TRG       0x0010
#define FPGA_RESET_WATCHDOG_TRG_VALUE 0xcafebabe

namespace saftlib {

Reset::Reset(etherbone::Device &dev) 
	: device(dev)
{
	// std::cerr << "Reset::Reset()" << std::endl;
	std::vector<sdb_device> reset_dev;
	device.sdb_find_by_identity(RESET_VENDOR_ID, RESET_DEVICE_ID, reset_dev);

	if (reset_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no reset device found on hardware");
	}
	if (reset_dev.size() > 1) {
		std::cerr << "more than one reset device found on hardware, taking the first one" << std::endl;
	}
	reset = static_cast<eb_address_t>(reset_dev[0].sdb_component.addr_first);
}

void Reset::WdRetrigger() 
{
	device.write(reset + FPGA_RESET_WATCHDOG_TRG, EB_DATA32, (eb_data_t)FPGA_RESET_WATCHDOG_TRG_VALUE);
}

} // namespace