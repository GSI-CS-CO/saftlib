/*  Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#ifndef SER_CLK_GEN_REGS_H
#define SER_CLK_GEN_REGS_H

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include "SdbDevice.hpp"

#include <stdint.h>

#define IO_SER_CLK_GEN_PRODUCT_ID    0x5f3eaf43
#define IO_SER_CLK_GEN_VENDOR_ID     0x00000651

#define IO_SER_CLK_GEN_BITS          8

#define IO_SER_CLK_GEN_DEBUG_MODE    0

namespace saftlib {


class SerdesClockGen : public SdbDevice 
{

	typedef enum
	{
		eSCK_selr      = 0x00,
		eSCK_perr      = 0x04,
		eSCK_perhir    = 0x08,
		eSCK_fracr     = 0x0c,
		eSCK_normmaskr = 0x10,
		eSCK_skipmaskr = 0x14,
		eSCK_phofslr   = 0x18,
		eSCK_phofshr   = 0x1c,
	} e_SerClkGen_RegisterArea;

	typedef struct SerClkGenControl
	{
		uint32_t period_integer;
		uint32_t period_high;
		uint32_t period_fraction;
		uint16_t bit_pattern_normal;
		uint16_t bit_pattern_skip;
		uint64_t phase_offset;
	} s_SerClkGenControl;

	// eb_address_t clkgen_address;
	// etherbone::Device &device;

	static void CalcClockParameters(double hi, double lo, uint64_t phase, struct SerClkGenControl *control);
public:
	SerdesClockGen(etherbone::Device &device);

	bool StartClock(int io_channel, int io_index, double high_phase, double low_phase, uint64_t phase_offset);
	bool StopClock(int io_channel, int io_index);
	bool ConfigureClock(int io_channel, int io_index, double high_phase, double low_phase, uint64_t phase_offset);
};

} // namespace

#endif
