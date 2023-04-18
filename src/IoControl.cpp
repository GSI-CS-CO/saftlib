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

#include "IoControl.hpp"

#include "io_control_regs.h"

#include "Io.hpp"

#include <saftbus/error.hpp>

#include <vector>
#include <iostream>

namespace saftlib {

IoControl::IoControl(etherbone::Device &device)
	: SdbDevice(device, IO_CONTROL_VENDOR_ID,     IO_CONTROL_PRODUCT_ID)
	, clkgen(device)
{
	/* Helpers */
	unsigned io_table_entries_id     = 0;
	unsigned io_table_iterator       = 0;
	unsigned io_table_entry_iterator = 0;
	unsigned io_table_data_raw       = 0;
	unsigned io_table_addr           = 0;
	unsigned io_GPIOTotal            = 0;
	unsigned io_LVDSTotal            = 0;
	unsigned io_FixedTotal           = 0;
	s_IOCONTROL_SetupField s_aIOCONTROL_SetupField[IO_GPIO_MAX+IO_LVDS_MAX+IO_FIXED_MAX];
	eb_data_t gpio_count_reg;
	eb_data_t lvds_count_reg;
	eb_data_t fixed_count_reg;
	eb_data_t get_param;
	etherbone::Cycle cycle;
	std::vector<sdb_device> ioctl, tlus;

	/* Get number of IOs */
	cycle.open(device);
	cycle.read(adr_first+eGPIO_Info, EB_DATA32, &gpio_count_reg);
	cycle.read(adr_first+eLVDS_Info, EB_DATA32, &lvds_count_reg);
	cycle.read(adr_first+eFIXED_Info, EB_DATA32, &fixed_count_reg);
	cycle.close();
	io_GPIOTotal  = (gpio_count_reg&IO_INFO_TOTAL_COUNT_MASK) >> IO_INFO_TOTAL_SHIFT;
	io_LVDSTotal  = (lvds_count_reg&IO_INFO_TOTAL_COUNT_MASK) >> IO_INFO_TOTAL_SHIFT;
	io_FixedTotal = fixed_count_reg;

	/* Read IO information */
	for (io_table_iterator = 0; io_table_iterator < (io_GPIOTotal*4 + io_LVDSTotal*4 + io_FixedTotal*4); io_table_iterator++)
	{
		/* Read memory region */
		io_table_addr = eIO_Map_Table_Begin + io_table_iterator*4;

		/* Open a new cycle and get the parameter */
		cycle.open(device);
		cycle.read(adr_first+io_table_addr, EB_DATA32, &get_param);
		cycle.close();
		io_table_data_raw = (unsigned) get_param;

		if(io_table_entry_iterator < 3) /* Get the IO name */
		{
			/* Convert uint32 to 4x unit8 */
			s_aIOCONTROL_SetupField[io_table_entries_id].uName[(io_table_entry_iterator*4)+0] = (unsigned char) ((io_table_data_raw&0xff000000)>>24);
			s_aIOCONTROL_SetupField[io_table_entries_id].uName[(io_table_entry_iterator*4)+1] = (unsigned char) ((io_table_data_raw&0x00ff0000)>>16);
			s_aIOCONTROL_SetupField[io_table_entries_id].uName[(io_table_entry_iterator*4)+2] = (unsigned char) ((io_table_data_raw&0x0000ff00)>>8);
			s_aIOCONTROL_SetupField[io_table_entries_id].uName[(io_table_entry_iterator*4)+3] = (unsigned char) ((io_table_data_raw&0x000000ff));
			io_table_entry_iterator++;
		}
		else /* Get all other IO information */
		{
			/* Convert uint32 to 4x unit8 */
			s_aIOCONTROL_SetupField[io_table_entries_id].uSpecial       = (unsigned char) ((io_table_data_raw&0xff000000)>>24);
			s_aIOCONTROL_SetupField[io_table_entries_id].uIndex         = (unsigned char) ((io_table_data_raw&0x00ff0000)>>16);
			s_aIOCONTROL_SetupField[io_table_entries_id].uIOCfgSpace    = (unsigned char) ((io_table_data_raw&0x0000ff00)>>8);
			s_aIOCONTROL_SetupField[io_table_entries_id].uLogicLevelRes = (unsigned char) ((io_table_data_raw&0x000000ff));
			io_table_entry_iterator = 0;
			io_table_entries_id ++;
		}
	}

	// std::cerr << "************** NUM IOS " << (io_GPIOTotal + io_LVDSTotal) << std::endl;
	// /* Create an action sink for each IO */
	unsigned eca_in = 0, eca_out = 0;

	for (io_table_iterator = 0; io_table_iterator < (io_GPIOTotal + io_LVDSTotal); io_table_iterator++)
	{
		/* Helpers */
		char * cIOName;
		unsigned direction      = 0;
		unsigned internal_id    = 0;
		unsigned channel        = 0;
		unsigned special        = 0;
		unsigned logic_level    = 0;
		bool oe_available       = false;
		bool term_available     = false;
		bool spec_out_available = false;
		bool spec_in_available  = false;

		/* Get properties */
		direction   = (s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_DIR_MASK) >> IO_CFG_FIELD_DIR_SHIFT;
		internal_id = (s_aIOCONTROL_SetupField[io_table_iterator].uIndex);
		channel     = (s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_INFO_CHAN_MASK) >> IO_CFG_FIELD_INFO_CHAN_SHIFT;
		special     = (s_aIOCONTROL_SetupField[io_table_iterator].uSpecial&IO_SPECIAL_PURPOSE_MASK) >> IO_SPECIAL_PURPOSE_SHIFT;
		logic_level = (s_aIOCONTROL_SetupField[io_table_iterator].uLogicLevelRes&IO_LOGIC_RES_FIELD_LL_MASK) >> IO_LOGIC_RES_FIELD_LL_SHIFT;

		/* Get available options */
		if ((s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_OE_MASK) >> IO_CFG_FIELD_OE_SHIFT)     { oe_available = true; }
		else                                                                                                            { oe_available = false; }
		if ((s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_TERM_MASK) >> IO_CFG_FIELD_TERM_SHIFT) { term_available = true; }
		else                                                                                                            { term_available = false; }
		if ((s_aIOCONTROL_SetupField[io_table_iterator].uSpecial&IO_SPECIAL_OUT_MASK) >> IO_SPECIAL_OUT_SHIFT)          { spec_out_available = true; }
		else                                                                                                            { spec_out_available = false; }
		if ((s_aIOCONTROL_SetupField[io_table_iterator].uSpecial&IO_SPECIAL_IN_MASK) >> IO_SPECIAL_IN_SHIFT)            { spec_in_available = true; }
		else                                                                                                            { spec_in_available = false; }

		/* Get IO name */
		cIOName = s_aIOCONTROL_SetupField[io_table_iterator].uName;
		std::string IOName = cIOName;

		/* Create the IO controller object */
		ios.push_back(Io(device, IOName, direction, channel, eca_in, eca_out, internal_id, special, logic_level, oe_available,
	 		term_available, spec_out_available, spec_in_available, adr_first, clkgen));

		if (direction == IO_CFG_FIELD_DIR_OUTPUT || direction == IO_CFG_FIELD_DIR_INOUT) ++eca_out;
		if (direction == IO_CFG_FIELD_DIR_INPUT  || direction == IO_CFG_FIELD_DIR_INOUT) ++eca_in;
	}
}

std::vector<Io> & IoControl::get_ios()
{
	return ios;
}



}
