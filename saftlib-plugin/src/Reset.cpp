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
#define FPGA_RESET_VENDOR            0x0651              // vendor ID
#define FPGA_RESET_PRODUCT           0x3a362063          // product ID
#define FPGA_RESET_VMAJOR            1                   // major revision
#define FPGA_RESET_VMINOR            3                   // minor revision

// register offsets
#define FPGA_RESET_RESET             0x0000              // reset register of FPGA (write), write 'deadbeef' to reset
#define FPGA_RESET_USERLM32_GET      0x0004              // get reset status of user lm32, one bit per CPU, bit 0 is CPU 0 (read)
#define FPGA_RESET_USERLM32_SET      0x0008              // puts user lm32 into RESET, one bit per CPU, bit 0 is CPU 0 (write)
#define FPGA_RESET_USERLM32_CLEAR    0x000c              // clears RESET of user lm32, one bit per CPU, bit 0 is CPU 0 (write)
#define FPGA_RESET_WATCHDOG_DISABLE  0x0004              // disables watchdog (write),    write 'cafebabe' to disable watchdog
                                                         //                               write 'cafebab0' to reenable watchdog
#define FPGA_RESET_WATCHDOG_STAT     0x000c              // reads watchdog stauts (read), read '1': watchdog enabled, '0': watchdog disabled
#define FPGA_RESET_WATCHDOG_TRG      0x0010              // retrigger watchdog (write),   write 'cafebabe' regularly to prevent auto-reset
                                                         //
#define FPGA_RESET_PHY_RESET         0x0014              // reset register of PHY and SFP (write/read)
#define FPGA_RESET_PHY_DROP_LINK_WR  0x0001              // drop link: main (White Rabbit) port
#define FPGA_RESET_PHY_DROP_LINK_AUX 0x0002              // drop link: auxiliary port
#define FPGA_RESET_PHY_SFP_DIS_WR    0x0004              // disable SFP: main (White Rabbit) port
#define FPGA_RESET_PHY_SFP_DIS_AUX   0x0008              // disable SFP: auxiliary port


#define FPGA_RESET_WATCHDOG_TRG_VALUE 0xcafebabe

namespace saftlib {

Reset::Reset(etherbone::Device &dev) 
	: device(dev)
{
	// std::cerr << "Reset::Reset()" << std::endl;
	std::vector<sdb_device> reset_dev;
	device.sdb_find_by_identity(FPGA_RESET_VENDOR, FPGA_RESET_PRODUCT, reset_dev);

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

void Reset::CpuHalt(unsigned idx) 
{
	device.write(reset + FPGA_RESET_USERLM32_SET, EB_DATA32, (eb_data_t)(1<<idx));
}

void Reset::CpuReset(unsigned idx) 
{
	device.write(reset + FPGA_RESET_USERLM32_CLEAR, EB_DATA32, (eb_data_t)(1<<idx));
}

uint32_t Reset::CpuHaltStatus() 
{
	eb_data_t    status;
	device.read(reset + FPGA_RESET_USERLM32_GET, EB_DATA32, &status);
	return (uint32_t)status;
}

} // namespace