#include "IoControl.hpp"

#include "io_control_regs.h"

#include <saftbus/error.hpp>

#include <vector>
#include <iostream>

namespace eb_plugin {

IoControl::IoControl(etherbone::Device &dev)
	: device(dev)
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
	std::vector<sdb_device> ioctl, tlus, clkgen;

	/* Find IO control module */
	device.sdb_find_by_identity(IO_CONTROL_VENDOR_ID,     IO_CONTROL_PRODUCT_ID,     ioctl);
	// tr->getDevice().sdb_find_by_identity(ECA_TLU_SDB_VENDOR_ID,    ECA_TLU_SDB_DEVICE_ID,     tlus);
	// tr->getDevice().sdb_find_by_identity(IO_SER_CLK_GEN_VENDOR_ID, IO_SER_CLK_GEN_PRODUCT_ID, clkgen);

	if (ioctl.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "No IO control module found");
	}
	if (ioctl.size() > 1) {
		std::cerr << "More than one IO control module found, take the first one" << std::endl;
	}
	//if (ioctl.size() != 1 || tlus.size() != 1 || clkgen.size() != 1) return -1;
	eb_address_t ioctl_address = ioctl[0].sdb_component.addr_first;
	// eb_address_t tlu = tlus[0].sdb_component.addr_first;
	// eb_address_t clkgen_address = clkgen[0].sdb_component.addr_first;

	/* Get number of IOs */
	cycle.open(device);
	cycle.read(ioctl_address+eGPIO_Info, EB_DATA32, &gpio_count_reg);
	cycle.read(ioctl_address+eLVDS_Info, EB_DATA32, &lvds_count_reg);
	cycle.read(ioctl_address+eFIXED_Info, EB_DATA32, &fixed_count_reg);
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
		cycle.read(ioctl_address+io_table_addr, EB_DATA32, &get_param);
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

	// /* Create an action sink for each IO */
	// unsigned eca_in = 0, eca_out = 0;

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

	// 	/* Create the IO controller object */
	// 	InoutImpl::ConstructorType impl_args = {
	// 		tr, channel, internal_id, special, logic_level, oe_available,
	// 		term_available, spec_out_available, spec_in_available, ioctl_address, clkgen_address };
	// 	std::shared_ptr<InoutImpl> impl(new InoutImpl(impl_args));

	// 	unsigned eca_channel = 0; // ECA channel 0 is always for IO
	// 	TimingReceiver::SinkKey key_in (eca_channel, eca_in);  // order: gpio_inout, gpio_in,  lvds_inout, lvds_in
	// 	TimingReceiver::SinkKey key_out(eca_channel, eca_out); // order: gpio_inout, gpio_out, lvds_inout, lvds_out

	// 	std::string input_path  = tr->getObjectPath() + "/inputs/"  + IOName;
	// 	std::string output_path = tr->getObjectPath() + "/outputs/" + IOName;
	// 	sigc::slot<void> nill;

	// 	/* Add sinks depending on their direction */
	// 	switch(direction)
	// 	{
	// 		case IO_CFG_FIELD_DIR_OUTPUT:
	// 		{
	// 			Output::ConstructorType out_args = { IOName, output_path, "", tr, eca_channel, eca_out, impl, nill };
	// 			actionSinks[key_out] = Output::create(out_args);
	// 			++eca_out;
	// 			break;
	// 		}
	// 		case IO_CFG_FIELD_DIR_INPUT:
	// 		{
	// 			Input::ConstructorType  in_args  = { IOName, input_path,  "", tr, tlu,         eca_in,  impl, nill };
	// 			eventSources[key_in] = Input::create(in_args);
	// 			++eca_in;
	// 			break;
	// 		}
	// 		case IO_CFG_FIELD_DIR_INOUT:
	// 		{
	// 			Output::ConstructorType out_args = { IOName, output_path, input_path,  tr, eca_channel, eca_out, impl, nill };
	// 			Input::ConstructorType  in_args  = { IOName, input_path,  output_path, tr, tlu,         eca_in,  impl, nill };
	// 			actionSinks[key_out] = Output::create(out_args);
	// 			eventSources[key_in] = Input::create(in_args);
	// 			++eca_out;
	// 			++eca_in;
	// 			break;
	// 		}
	// 		default:
	// 		{
	// 			clog << kLogErr << "Found IO with unknown direction!" << std::endl;
	// 			return -1;
	// 			break;
	// 		}
	// 	}
	}
}



}
