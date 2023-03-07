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
#include <fstream>

#define LM32_RAM_USER_VENDOR      0x0651             // vendor ID
#define LM32_RAM_USER_PRODUCT     0x54111351         // product ID
#define LM32_RAM_USER_VMAJOR      1                  // major revision
#define LM32_RAM_USER_VMINOR      0                  // minor revision

#define LM32_CLUSTER_ROM_VENDOR   0x651
#define LM32_CLUSTER_ROM_PRODUCT  0x10040086

namespace saftlib {


LM32Cluster::LM32Cluster(etherbone::Device &dev, TimingReceiver *timing_receiver) 
	: SdbDevice(dev, LM32_CLUSTER_ROM_VENDOR, LM32_CLUSTER_ROM_PRODUCT)
	, tr(timing_receiver)
{
	std::cerr << "LM32Cluster::LM32Cluster" << std::endl;

    eb_data_t cpus;
    device.read(adr_first, EB_DATA32, &cpus);

	// look for lm32 dual port ram
	std::vector<sdb_device> dpram_lm32_devs;
	device.sdb_find_by_identity(LM32_RAM_USER_VENDOR, LM32_RAM_USER_PRODUCT, dpram_lm32_devs);

	if (dpram_lm32_devs.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no lm32 user ram found on hardware");
	}

	for (auto& dpram_lm32_dev: dpram_lm32_devs) {
		dpram_lm32_adr_first.push_back(static_cast<eb_address_t>(dpram_lm32_dev.sdb_component.addr_first));
		dpram_lm32_adr_last.push_back(static_cast<eb_address_t>(dpram_lm32_dev.sdb_component.addr_last));
	}

	num_cores = dpram_lm32_devs.size();

	if (num_cores != cpus) {
		throw saftbus::Error(saftbus::Error::FAILED, "number of cpus in lm32 cluster rom differs from number of number of user rams");
	}

	std::cerr << "found " << num_cores << " lm32 cpus" << std::endl;
}
LM32Cluster::~LM32Cluster() {
}

unsigned LM32Cluster::getCpuCount()
{
	std::cerr << "getCpuCount" << std::endl;
	return dpram_lm32_adr_first.size();
}

void LM32Cluster::SafeHaltCpu(unsigned cpu_idx)
{
	if (cpu_idx >= num_cores) {
		std::ostringstream msg;
		msg << "there is no user cpu core with index " << cpu_idx;
		throw std::runtime_error(msg.str());	
	}
	std::cerr << "safeHaltCpu " << cpu_idx << std::endl;
	// overwrite the RAM with trap instructions (a trap instruction is a jump to the address of the flummi instruction)
	eb_address_t adr = dpram_lm32_adr_first[cpu_idx];
	eb_address_t last = dpram_lm32_adr_last[cpu_idx];
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
	tr->CpuHalt(cpu_idx);
}

void LM32Cluster::WriteFirmware(unsigned cpu_idx, const std::string &filename)
{
	if (cpu_idx >= num_cores) {
		std::ostringstream msg;
		msg << "there is no user cpu core with index " << cpu_idx;
		throw std::runtime_error(msg.str());	
	}
	eb_address_t adr = dpram_lm32_adr_first[cpu_idx];
	eb_address_t last = dpram_lm32_adr_last[cpu_idx];
	std::ifstream firmware_bin(filename.c_str());
	if (!firmware_bin) {
		std::ostringstream msg;
		msg << "cannot open firmware binary file " << filename;
		throw std::runtime_error(msg.str());
	}

	bool end_of_file = false;
	while (adr < last) {
		std::cerr << adr << std::endl;
		etherbone::Cycle cycle;
		cycle.open(device);
		for (int i = 0; i < 32 && adr < last; ++i) {
			uint32_t instr, cpu_instr = 0;
			firmware_bin.read((char*)&instr, sizeof(instr));
			if (!firmware_bin) {
				end_of_file = true;
				break;
			}
			cpu_instr |= instr >> 24;
			cpu_instr |= (0xff0000 & instr) >> 8;
			cpu_instr |= (0x00ff00 & instr) << 8;
			cpu_instr |= instr << 24;
			cycle.write(adr, EB_DATA32, (eb_data_t)cpu_instr);
			adr += 4;
		}
		cycle.close();
		if (end_of_file) {
			break;
		}
	}
}

} // namespace