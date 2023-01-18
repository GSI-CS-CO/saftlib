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
#include "TimingReceiver.hpp"

#include <saftbus/error.hpp>

#include <iostream>
#include <cassert>
#include <sstream>

#define LM32_RAM_USER_VENDOR      0x0651             // vendor ID
#define LM32_RAM_USER_PRODUCT     0x54111351         // product ID
#define LM32_RAM_USER_VMAJOR      1                  // major revision
#define LM32_RAM_USER_VMINOR      0                  // minor revision


namespace saftlib {

class LM32Firmware;

LM32Cluster::LM32Cluster(etherbone::Device &dev, TimingReceiver *timing_receiver) 
	: device(dev)
	, tr(timing_receiver)
{
	std::cerr << "LM32Cluster::LM32Cluster" << std::endl;
	// look for lm32 dual port ram
	std::vector<sdb_device> dpram_lm32_devs;
	device.sdb_find_by_identity(LM32_RAM_USER_VENDOR, LM32_RAM_USER_PRODUCT, dpram_lm32_devs);

	if (dpram_lm32_devs.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no lm32 user ram found on hardware");
	}

	for (auto& dpram_lm32_dev: dpram_lm32_devs) {
		dpram_lm32.push_back(static_cast<eb_address_t>(dpram_lm32_dev.sdb_component.addr_first));
		dpram_lm32_last.push_back(static_cast<eb_address_t>(dpram_lm32_dev.sdb_component.addr_last));
	}

	num_cores = dpram_lm32.size();
	firmware_drivers.resize(num_cores);

	std::cerr << "found " << dpram_lm32.size() << " lm32 cpus" << std::endl;

	int result = 0;//lt_dlinit();
	assert(result == 0);
}
LM32Cluster::~LM32Cluster() {
	int result = 0;//lt_dlexit();
	assert(result == 0);
}

void LM32Cluster::load_plugin(const std::string &filename) 
{
	auto plugin = plugins.find(filename);
	if (plugin != plugins.end()) {
		// open the file
		// lt_dlhandle handle = lt_dlopen(filename.c_str());
		void * handle = dlopen(filename.c_str(), RTLD_NOW|RTLD_GLOBAL);
		if (handle == nullptr) {
			std::ostringstream msg;
			msg << "cannot load firmware driver plugin: failed to open file " << filename;
			throw std::runtime_error(msg.str());
		}
		// load the function pointer
		// attach_firmware_driver_function function = (attach_firmware_driver_function)lt_dlsym(handle, "attach_firmware_driver");
		attach_firmware_driver_function function = (attach_firmware_driver_function)dlsym(handle, "attach_firmware_driver");
		if (function == nullptr) {
			// lt_dlclose(handle);
			dlclose(handle);
			throw std::runtime_error("cannot load plugin because symbol \"attach_firmware_driver\" cannot be loaded");
		}
		plugins[filename].handle                 = handle;
		plugins[filename].attach_firmware_driver = function;
	}
}

unsigned LM32Cluster::getCpuCount()
{
	std::cerr << "getCpuCount" << std::endl;
	return dpram_lm32.size();
}

void LM32Cluster::safeHaltCpu(unsigned cpu_idx)
{
	std::cerr << "safeHaltCpu " << cpu_idx << std::endl;
	tr->CpuHalt(cpu_idx);
	// overwrite the RAM with trap instructions (a trap instruction is a jump to the address of the flummi instruction)
	eb_address_t adr = dpram_lm32[cpu_idx];
	eb_address_t last = dpram_lm32_last[cpu_idx];
	eb_data_t jump_instruction = 0xe0000000;
	std::cerr << std::hex << adr << " " << last << std::endl;
	while (adr < last) {
		std::cerr << ".";
		etherbone::Cycle cycle;
		cycle.open(device);
		for (int i = 0; i < 32 && adr < last; ++i) {
			cycle.write(adr, EB_DATA32, (eb_data_t)jump_instruction);
			adr += 4;
		}
		cycle.close();
	}
	
	tr->CpuReset(cpu_idx);
	tr->CpuHalt(cpu_idx);
}



// void LM32Cluster::AttachFirwareDriver(unsigned idx, const std::string &filename)
// {
// 	if (idx >= 0 && idx <= num_cores) {
// 		firmware_drivers[idx] = std::move(firmware_driver);
// 	} else {
// 		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "invalid cpu index");
// 	}
// }


} // namespace