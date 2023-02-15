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

#ifndef SAFTLIB_RESET_HPP_
#define SAFTLIB_RESET_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include "SdbDevice.hpp"

namespace saftlib {

class Reset : public SdbDevice {
public:
	Reset(etherbone::Device &device);
	/// @brief retrigger reset watchdog. 
	/// 
	/// If reset watchdog is enabled and not retriggered within 10 minutes it will reset the FPGA
	/// 
	// @saftbus-export
	void WdRetrigger();

	/// @brief permanently assert reset line of cpu[idx]
	/// @param idx halt cpu[idx] (no check if idx is valid)
	/// 
	// @saftbus-export
	void CpuHalt(unsigned idx);

	/// @brief release reset line of cpu[idx]
	/// @param idx cpu[idx] is reset and stats excecuting its program (no check if idx is valid)
	/// 
	// @saftbus-export
	void CpuReset(unsigned idx);

	/// @brief get the 'halt status' of all user lm32 (rightmost bit: CPU 0). bit='1' means halted.
	/// @return 32 bits, where bit at position idx represents the halt status of cpu[idx]. '1' means haltet.
	///
	// @saftbus-export
	uint32_t CpuHaltStatus();
};

}

#endif
