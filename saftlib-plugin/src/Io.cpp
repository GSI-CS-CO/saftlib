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

#include "Io.hpp"

#include "io_control_regs.h"
#include "eca_tlu_regs.h"
#include "SerdesClockGen.hpp"

#include <saftbus/error.hpp>

namespace saftlib {

Io::Io(etherbone::Device &dev
	 , const std::string &name
	 , unsigned direction
	 , unsigned channel
	 , unsigned index
	 , unsigned special_purpose
	 , unsigned logic_level
	 , bool oe_available
	 , bool term_available
	 , bool spec_out_available
	 , bool spec_in_available
	 , eb_address_t control_addr
	 , SerdesClockGen &clkgen ) 
	: device(dev)
	, io_name(name)
	, io_direction(direction)
	, io_channel(channel)
	, io_index(index)
	, io_special_purpose(special_purpose)
	, io_logic_level(logic_level)
	, io_oe_available(oe_available)
	, io_term_available(term_available)
	, io_spec_out_available(spec_out_available)
	, io_spec_in_available(spec_in_available)
	, io_control_addr(control_addr)
	, io_clkgen(clkgen)
{}


const std::string &Io::getName() const {
	return io_name;
}

unsigned Io::getDirection() const {
	return io_direction;
}

uint32_t Io::getIndexOut() const
{
	return io_index;
}

uint32_t Io::getIndexIn() const
{
	return io_index;
}

void Io::WriteOutput(bool value)
{
	etherbone::Cycle cycle;
	eb_data_t writeOutput;

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (value == true) { writeOutput = 0x01; }
		else               { writeOutput = 0x00; }
		cycle.write(io_control_addr+eSet_GPIO_Out_Begin+(io_index*4), EB_DATA32, writeOutput);
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (value == true) { writeOutput = 0xff; }
		else               { writeOutput = 0x00; }
		cycle.write(io_control_addr+eSet_LVDS_Out_Begin+(io_index*4), EB_DATA32, writeOutput);
	}
	else
	{
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!");
	}

	cycle.close();
}

bool Io::ReadOutput()
{
	etherbone::Cycle cycle;
	eb_data_t readOutput;

	cycle.open(device);
	if      (io_channel == IO_CFG_CHANNEL_GPIO) { cycle.read(io_control_addr+eSet_GPIO_Out_Begin+(io_index*4), EB_DATA32, &readOutput); }
	else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eSet_LVDS_Out_Begin+(io_index*4), EB_DATA32, &readOutput); }
	else                                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	if(readOutput) { return true; }
	else           { return false; }
}

bool Io::ReadCombinedOutput()
{
	etherbone::Cycle cycle;
	eb_data_t readCombinedOutput;
	eb_data_t inputOffset;
	unsigned inputRegOffset = 0;
	unsigned totalInputs = 0;

	cycle.open(device);
	if      (io_channel == IO_CFG_CHANNEL_GPIO) { cycle.read(io_control_addr+eGPIO_Info, EB_DATA32, &inputOffset); }
	else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eLVDS_Info, EB_DATA32, &inputOffset); }
	else                                   { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();
	totalInputs = (unsigned) inputOffset;
	totalInputs = ((totalInputs&IO_INFO_IN_COUNT_MASK) >> IO_INFO_IN_SHIFT) + ((totalInputs&IO_INFO_INOUT_COUNT_MASK) >> IO_INFO_INOUT_SHIFT);
	inputRegOffset = totalInputs*4;
	
	cycle.open(device);
	if      (io_channel == IO_CFG_CHANNEL_GPIO) { cycle.read(io_control_addr+eGet_GPIO_In_Begin+inputRegOffset+(io_index*4), EB_DATA32, &readCombinedOutput); }
	else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eGet_LVDS_In_Begin+inputRegOffset+(io_index*4), EB_DATA32, &readCombinedOutput); }
	else                                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	if(readCombinedOutput) { return true; }
	else                   { return false; }
}

bool Io::getOutputEnable() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readOutputEnable;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_Oe_Set_low,  EB_DATA32, &readOutputEnable); }
		else                      { cycle.read(io_control_addr+eGPIO_Oe_Set_high, EB_DATA32, &readOutputEnable); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_Oe_Set_low,  EB_DATA32, &readOutputEnable); }
		else                      { cycle.read(io_control_addr+eLVDS_Oe_Set_high, EB_DATA32, &readOutputEnable); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readOutputEnable = readOutputEnable&(1<<internal_id);
	readOutputEnable = readOutputEnable>>internal_id;

	if(readOutputEnable) { return true; }
	else                 { return false; }
}

void Io::setOutputEnable(bool val)
{
	etherbone::Cycle cycle;

	unsigned id_low  = io_index % 32;
	unsigned id_high = io_index / 32;
	eb_data_t id_mask = eb_data_t(1) << id_low;

	eb_address_t reg;
	if (io_channel == IO_CFG_CHANNEL_GPIO) {
		reg = val?eGPIO_Oe_Set_low:eGPIO_Oe_Reset_low;
	} else if (io_channel == IO_CFG_CHANNEL_LVDS) {
		reg = val?eLVDS_Oe_Set_low:eLVDS_Oe_Reset_low;
	} else {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!");
	}

	device.write(io_control_addr + reg + 4*id_high, EB_DATA32, id_mask);
	if (OutputEnable) {
		OutputEnable(val);
	}
}

bool Io::ReadInput()
{
	etherbone::Cycle cycle;
	eb_data_t readInput;

	cycle.open(device);
	if      (io_channel == IO_CFG_CHANNEL_GPIO) { cycle.read(io_control_addr+eGet_GPIO_In_Begin+(io_index*4), EB_DATA32, &readInput); }
	else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eGet_LVDS_In_Begin+(io_index*4), EB_DATA32, &readInput); }
	else                                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	if (readInput) { return true; }
	else           { return false; }
}

bool Io::getInputTermination() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readInputTermination;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_Term_Set_low,  EB_DATA32, &readInputTermination); }
		else                      { cycle.read(io_control_addr+eGPIO_Term_Set_high, EB_DATA32, &readInputTermination); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_Term_Set_low,  EB_DATA32, &readInputTermination); }
		else                      { cycle.read(io_control_addr+eLVDS_Term_Set_high, EB_DATA32, &readInputTermination); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readInputTermination = readInputTermination&(1<<internal_id);
	readInputTermination = readInputTermination>>internal_id;

	if (readInputTermination) { return true; }
	else                      { return false; }
}

void Io::setInputTermination(bool val)
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Term_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Term_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Term_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Term_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Term_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Term_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Term_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Term_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	cycle.close();
	if (InputTermination) {
		InputTermination(val);
	}
}

bool Io::getOutputEnableAvailable() const
{
	return io_oe_available;
}

bool Io::getSpecialPurposeOutAvailable() const
{
	return io_spec_out_available;
}

bool Io::getInputTerminationAvailable() const
{
	return io_term_available;
}

bool Io::getSpecialPurposeInAvailable() const
{
	return io_spec_in_available;
}

bool Io::getSpecialPurposeOut() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readSpecialPurposeOut;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_Spec_Out_Set_low,  EB_DATA32, &readSpecialPurposeOut); }
		else                      { cycle.read(io_control_addr+eGPIO_Spec_Out_Set_high, EB_DATA32, &readSpecialPurposeOut); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_Spec_Out_Set_low,  EB_DATA32, &readSpecialPurposeOut); }
		else                      { cycle.read(io_control_addr+eLVDS_Spec_Out_Set_high, EB_DATA32, &readSpecialPurposeOut); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readSpecialPurposeOut = readSpecialPurposeOut&(1<<internal_id);
	readSpecialPurposeOut = readSpecialPurposeOut>>internal_id;

	if (readSpecialPurposeOut) { return true; }
	else                       { return false; }
}

void Io::setSpecialPurposeOut(bool val)
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Spec_Out_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Spec_Out_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Spec_Out_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Spec_Out_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Spec_Out_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Spec_Out_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Spec_Out_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Spec_Out_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	cycle.close();
	if (SpecialPurposeOut) {
		SpecialPurposeOut(val);
	}
}

bool Io::getGateOut() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readGateOut;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_Gate_Out_Set_low,  EB_DATA32, &readGateOut); }
		else                      { cycle.read(io_control_addr+eGPIO_Gate_Out_Set_high, EB_DATA32, &readGateOut); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_Gate_Out_Set_low,  EB_DATA32, &readGateOut); }
		else                      { cycle.read(io_control_addr+eLVDS_Gate_Out_Set_high, EB_DATA32, &readGateOut); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readGateOut = readGateOut&(1<<internal_id);
	readGateOut = readGateOut>>internal_id;

	if (readGateOut) { return true; }
	else                       { return false; }
}

void Io::setGateOut(bool val)
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Gate_Out_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Gate_Out_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Gate_Out_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Gate_Out_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Gate_Out_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Gate_Out_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Gate_Out_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Gate_Out_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	cycle.close();
	//if (GateOut) {
	//	GateOut(val);
	//}
}

bool Io::getSpecialPurposeIn() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readSpecialPurposeIn;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_Spec_In_Set_low,  EB_DATA32, &readSpecialPurposeIn); }
		else                      { cycle.read(io_control_addr+eGPIO_Spec_In_Set_high, EB_DATA32, &readSpecialPurposeIn); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_Spec_In_Set_low,  EB_DATA32, &readSpecialPurposeIn); }
		else                      { cycle.read(io_control_addr+eLVDS_Spec_In_Set_high, EB_DATA32, &readSpecialPurposeIn); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readSpecialPurposeIn = readSpecialPurposeIn&(1<<internal_id);
	readSpecialPurposeIn = readSpecialPurposeIn>>internal_id;

	if (readSpecialPurposeIn) { return true; }
	else                      { return false; }
}

void Io::setSpecialPurposeIn(bool val)
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Spec_In_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Spec_In_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Spec_In_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Spec_In_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Spec_In_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Spec_In_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Spec_In_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Spec_In_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	cycle.close();
	if (SpecialPurposeIn) {
		SpecialPurposeIn(val);
	}
}

bool Io::getGateIn() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readGateIn;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_Gate_In_Set_low,  EB_DATA32, &readGateIn); }
		else                      { cycle.read(io_control_addr+eGPIO_Gate_In_Set_high, EB_DATA32, &readGateIn); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_Gate_In_Set_low,  EB_DATA32, &readGateIn); }
		else                      { cycle.read(io_control_addr+eLVDS_Gate_In_Set_high, EB_DATA32, &readGateIn); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readGateIn = readGateIn&(1<<internal_id);
	readGateIn = readGateIn>>internal_id;

	if (readGateIn) { return true; }
	else                       { return false; }
}

void Io::setGateIn(bool val)
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Gate_In_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Gate_In_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Gate_In_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Gate_In_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Gate_In_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Gate_In_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Gate_In_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Gate_In_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	cycle.close();
	if (GateIn) {
		GateIn(val);
	}
}



bool Io::getBuTiSMultiplexer() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readBuTiSMultiplexer;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_Mux_Set_low,  EB_DATA32, &readBuTiSMultiplexer); }
		else                      { cycle.read(io_control_addr+eGPIO_Mux_Set_high, EB_DATA32, &readBuTiSMultiplexer); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_Mux_Set_low,  EB_DATA32, &readBuTiSMultiplexer); }
		else                      { cycle.read(io_control_addr+eLVDS_Mux_Set_high, EB_DATA32, &readBuTiSMultiplexer); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readBuTiSMultiplexer = readBuTiSMultiplexer&(1<<internal_id);
	readBuTiSMultiplexer = readBuTiSMultiplexer>>internal_id;

	if (readBuTiSMultiplexer) { return true; }
	else                      { return false; }
}

void Io::setBuTiSMultiplexer(bool val)
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Mux_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Mux_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Mux_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_Mux_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Mux_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Mux_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Mux_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_Mux_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	cycle.close();
	if (BuTiSMultiplexer) {
		BuTiSMultiplexer(val);
	}
}

bool Io::getPPSMultiplexer() const
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	eb_data_t readPPSMultiplexer;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eGPIO_PPS_Mux_Set_low,  EB_DATA32, &readPPSMultiplexer); }
		else                      { cycle.read(io_control_addr+eGPIO_PPS_Mux_Set_high, EB_DATA32, &readPPSMultiplexer); }
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (access_position == 0) { cycle.read(io_control_addr+eLVDS_PPS_Mux_Set_low,  EB_DATA32, &readPPSMultiplexer); }
		else                      { cycle.read(io_control_addr+eLVDS_PPS_Mux_Set_high, EB_DATA32, &readPPSMultiplexer); }
	}
	else                        { throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel unknown!"); }
	cycle.close();

	readPPSMultiplexer = readPPSMultiplexer&(1<<internal_id);
	readPPSMultiplexer = readPPSMultiplexer>>internal_id;

	if (readPPSMultiplexer) { return true; }
	else                      { return false; }
}

void Io::setPPSMultiplexer(bool val)
{
	unsigned access_position = 0;
	unsigned internal_id = io_index;
	etherbone::Cycle cycle;

	/* Calculate access position (32bit access to 64bit register)*/
	if (io_index>31)
	{
		internal_id = io_index-31;
		access_position = 1;
	}

	cycle.open(device);
	if (io_channel == IO_CFG_CHANNEL_GPIO)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_PPS_Mux_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_PPS_Mux_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eGPIO_PPS_Mux_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eGPIO_PPS_Mux_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	else if (io_channel == IO_CFG_CHANNEL_LVDS)
	{
		if (val)
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_PPS_Mux_Set_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_PPS_Mux_Set_high, EB_DATA32, (1<<internal_id)); }
		}
		else
		{
			if (access_position == 0) { cycle.write(io_control_addr+eLVDS_PPS_Mux_Reset_low,  EB_DATA32, (1<<internal_id)); }
			else                      { cycle.write(io_control_addr+eLVDS_PPS_Mux_Reset_high, EB_DATA32, (1<<internal_id)); }
		}
	}
	cycle.close();
	if (PPSMultiplexer) {
		PPSMultiplexer(val);
	}
}

bool Io::StartClock(double high_phase, double low_phase, uint64_t phase_offset) { 
	return io_clkgen.StartClock(io_channel, io_index, high_phase, low_phase, phase_offset); 
}

bool Io::StopClock() { 
	return io_clkgen.StopClock(io_channel, io_index); 
}

std::string Io::getLogicLevelOut() const { return getLogicLevel(); }
std::string Io::getLogicLevelIn() const { return getLogicLevel(); }
std::string Io::getLogicLevel() const
{
	std::string IOLogicLevel;

	switch(io_logic_level)
	{
		case IO_LOGIC_LEVEL_TTL:   { // display the correct level if the special function is level conversion
			IOLogicLevel = "TTL";   
			if (io_special_purpose == IO_SPECIAL_TTL_TO_NIM) {
				if ( (io_spec_out_available && getSpecialPurposeOut()) || (io_spec_in_available  && getSpecialPurposeIn())) {
					IOLogicLevel = "NIM";
				} 
			} 
		}
		break; 
		case IO_LOGIC_LEVEL_LVTTL: { IOLogicLevel = "LVTTL"; break; }
		case IO_LOGIC_LEVEL_LVDS:  { IOLogicLevel = "LVDS";  break; }
		case IO_LOGIC_LEVEL_NIM:   { // display the correct level if the special function is level conversion
			IOLogicLevel = "NIM";   
			if (io_special_purpose == IO_SPECIAL_TTL_TO_NIM) {
				if ( (io_spec_out_available && getSpecialPurposeOut()) || (io_spec_in_available  && getSpecialPurposeIn())) {
					IOLogicLevel = "TTL";
				} 
			} 
		}
		break; 
		case IO_LOGIC_LEVEL_CMOS:  { IOLogicLevel = "CMOS";  break; }
		default:                   { IOLogicLevel = "?";     break; }
	}

	return IOLogicLevel;
}

std::string Io::getTypeOut() const { return getType(); }
std::string Io::getTypeIn() const { return getType(); }
std::string Io::getType() const
{
	std::string IOType;

	switch(io_channel)
	{
		case IO_CFG_CHANNEL_GPIO: { IOType = "8ns (GPIO)"; break; }
		case IO_CFG_CHANNEL_LVDS: { IOType = "1ns (LVDS)"; break; }
		default: throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel type unknown!");
	}

	return IOType;
}

uint64_t Io::getResolution() const
{
	switch (io_channel) {
	case IO_CFG_CHANNEL_GPIO: return 8;
	case IO_CFG_CHANNEL_LVDS: return 1;
	default: throw saftbus::Error(saftbus::Error::INVALID_ARGS, "IO channel resolution unknown!");
	}
}



} // namespace 



