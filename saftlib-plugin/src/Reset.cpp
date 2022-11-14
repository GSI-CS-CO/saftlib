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

// this should later #include "bel_projects/tools/wb_slaves.h"
// for now, just redefine the register offsets
#define RESET_VENDOR_ID         0x651
#define RESET_DEVICE_ID         0x3a362063

#define FPGA_RESET_WATCHDOG_TRG       0x0010
#define FPGA_RESET_WATCHDOG_TRG_VALUE 0xcafebabe


#define FPGA_RESET_RESET             0x0000              // reset register of FPGA (write)
#define FPGA_RESET_USERLM32_GET      0x0004              // get reset status of user lm32, one bit per CPU, bit 0 is CPU 0 (read)
#define FPGA_RESET_USERLM32_SET      0x0008              // puts user lm32 into RESET, one bit per CPU, bit 0 is CPU 0 (write)
#define FPGA_RESET_USERLM32_CLEAR    0x000c              // clears RESET of user lm32, one bit per CPU, bit 0 is CPU 0 (write)
#define FPGA_RESET_WATCHDOG_DISABLE  0x0004              // disables watchdog (write), write 'cafebabe' to prevent auto-restart


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

void Reset::CpuHalt(int idx) 
{

}

void Reset::CpuReset(int idx) 
{

}

uint32_t Reset::CpuHaltStatus() 
{
	eb_data_t    status;
	device.read(reset + FPGA_RESET_USERLM32_GET, EB_DATA32, &status);
	return (uint32_t)status;
}

} // namespace