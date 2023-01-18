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

#ifndef SAFTLIB_LM32CLUSTER_HPP_
#define SAFTLIB_LM32CLUSTER_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <vector>
#include <map>
#include <memory>
// #include <ltdl.h>
#include <dlfcn.h>

// @saftbus-export
#include "LM32Firmware.hpp"
// @saftbus-export
#include <saftbus/service.hpp>

namespace saftlib {
	class LM32Cluster;
	class TimingReceiver;
}

// extern "C" 
// typedef void (*attach_firmware_driver_function) (saftbus::Container *container, saftlib::LM32Cluster *lm32_cluster, saftlib::TimingReceiver *tr, const std::vector<std::string> &args);
extern "C" typedef void (*attach_firmware_driver_function) (saftbus::Container *container, const std::vector<std::string> &args);

namespace saftlib {



class LM32Cluster {
	etherbone::Device &device;

	std::vector<eb_address_t>                   dpram_lm32;
	std::vector<eb_address_t>                   dpram_lm32_last;
	std::vector<std::unique_ptr<LM32Firmware> > firmware_drivers;

	struct FirmwarePlugin {
		// lt_dlhandle handle;
		void * handle;
		attach_firmware_driver_function attach_firmware_driver;
	};

	std::map<std::string, FirmwarePlugin> plugins;


	unsigned num_cores;
	unsigned ram_per_core;
	bool is_dm;
	TimingReceiver *tr;
public:
	LM32Cluster(etherbone::Device &device, TimingReceiver *tr);
	~LM32Cluster();

	void load_plugin(const std::string &filename);

	/// @brief number of CPUs
	/// @return number of instanciated User LM32 Cores
	/// 
	// @saftbus-export
	unsigned getCpuCount();

	/// @brief stop execution of cpu[cpu_idx]
	/// @param cpu_idx identifies the cpu that should be halted 
	///
	/// To avoid halting the cpu inside a whishbone cycle, the ram is filled with jump instructions that jump to the same location.
	// @saftbus-export
	void safeHaltCpu(unsigned cpu_idx);


	// void AttachFirwareDriver(unsigned idx, std::unique_ptr<LM32Firmware> &firmware_driver);
};

}

#endif
