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

namespace saftlib {

class LM32Cluster {
	etherbone::Device &device;

	std::vector<eb_address_t> dpram_lm32;

    int num_cores;
    int ram_per_core;
    bool is_dm;
public:
	LM32Cluster(etherbone::Device &device);
	/// @brief retrigger reset watchdog. 
	/// 
	/// If reset watchdog is enabled and not retriggered within 10 minutes it will reset the FPGA
	/// 
	// @saftbus-export
	void WdRetrigger();
};

}

#endif
