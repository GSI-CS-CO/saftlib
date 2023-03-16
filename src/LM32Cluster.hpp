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
#include <string>
#include <map>
#include <memory>
// #include <ltdl.h>
#include <dlfcn.h>

// @saftbus-export
#include <saftbus/service.hpp>

#include <SdbDevice.hpp>

namespace saftlib {

class TimingReceiver;

class LM32Cluster : public SdbDevice {

	unsigned num_cores;
	unsigned ram_per_core;
	TimingReceiver *tr;
public:
	LM32Cluster(etherbone::Device &device, TimingReceiver *tr);
	~LM32Cluster();

	std::vector<eb_address_t> dpram_lm32_adr_first;
	std::vector<eb_address_t> dpram_lm32_adr_last;

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
	void SafeHaltCpu(unsigned cpu_idx);

	void WriteFirmware(unsigned cpu_idx, const std::string &filename);
};

}

#endif
