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

#include "LM32Cluster.hpp"

#include <saftbus/error.hpp>

#include <iostream>

#define DPRAMLM32_VENDOR_ID         0x651
#define DPRAMLM32_DEVICE_ID         0x54111351

namespace saftlib {

class LM32Firmware;

LM32Cluster::LM32Cluster(etherbone::Device &dev) 
	: device(dev)
{
	// look for lm32 dual port ram
	std::vector<sdb_device> dpram_lm32_devs;
	device.sdb_find_by_identity(DPRAMLM32_VENDOR_ID, DPRAMLM32_DEVICE_ID, dpram_lm32_devs);

	if (dpram_lm32_devs.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no lm32 user ram found on hardware");
	}

    for (auto& dpram_lm32_dev: dpram_lm32_devs) {
        dpram_lm32.push_back(static_cast<eb_address_t>(dpram_lm32_dev.sdb_component.addr_first));
    }

    num_cores = dpram_lm32.size();
    firmware_drivers.resize(num_cores);

    std::cerr << "found " << dpram_lm32.size() << " lm32 cpus" << std::endl;

}

unsigned LM32Cluster::getCpuCount()
{
	return dpram_lm32.size();
}


void LM32Cluster::AttachFirwareDriver(unsigned idx, std::unique_ptr<LM32Firmware> &firmware_driver)
{
    if (idx >= 0 && idx <= num_cores) {
        firmware_drivers[idx] = std::move(firmware_driver);
    } else {
        throw saftbus::Error(saftbus::Error::INVALID_ARGS, "invalid cpu index");
    }
}


} // namespace