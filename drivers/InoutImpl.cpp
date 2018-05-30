/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include "InoutImpl.h"
#include "io_control_regs.h"
#include "eca_tlu_regs.h"
#include "src/clog.h"
#include "Output.h"
#include "Input.h"
#include "SerdesClockGen.h"

namespace saftlib {

InoutImpl::InoutImpl(const ConstructorType& args)
 : tr(args.tr),
   io_channel(args.io_channel), io_index(args.io_index), io_special_purpose(args.io_special_purpose), io_logic_level(args.io_logic_level),
   io_oe_available(args.io_oe_available), io_term_available(args.io_term_available),
   io_spec_out_available(args.io_spec_out_available), io_spec_in_available(args.io_spec_in_available),
   io_control_addr(args.io_control_addr), io_ser_clk_gen_addr(args.io_ser_clk_gen_addr)
{
}

void InoutImpl::WriteOutput(bool value)
{
  etherbone::Cycle cycle;
  eb_data_t writeOutput;
  
  cycle.open(tr->getDevice());
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
    throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!");
  }
  
  cycle.close();
}

bool InoutImpl::ReadOutput()
{
  etherbone::Cycle cycle;
  eb_data_t readOutput;
  
  cycle.open(tr->getDevice());
  if      (io_channel == IO_CFG_CHANNEL_GPIO) { cycle.read(io_control_addr+eSet_GPIO_Out_Begin+(io_index*4), EB_DATA32, &readOutput); }
  else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eSet_LVDS_Out_Begin+(io_index*4), EB_DATA32, &readOutput); }
  else                                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
  
  if(readOutput) { return true; }
  else           { return false; }
}

bool InoutImpl::getOutputEnable() const
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
  
  cycle.open(tr->getDevice());
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
  else                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
  
  readOutputEnable = readOutputEnable&(1<<internal_id);
  readOutputEnable = readOutputEnable>>internal_id;
  
  if(readOutputEnable) { return true; }
  else                 { return false; }
}

void InoutImpl::setOutputEnable(bool val)
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
    throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!");
  }
  
  tr->getDevice().write(io_control_addr + reg + 4*id_high, EB_DATA32, id_mask);
  OutputEnable(val);
}

bool InoutImpl::ReadInput()
{
  etherbone::Cycle cycle;
  eb_data_t readInput;
  
  cycle.open(tr->getDevice());
  if      (io_channel == IO_CFG_CHANNEL_GPIO) { cycle.read(io_control_addr+eGet_GPIO_In_Begin+(io_index*4), EB_DATA32, &readInput); }
  else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eGet_LVDS_In_Begin+(io_index*4), EB_DATA32, &readInput); }
  else                                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
  
  if (readInput) { return true; }
  else           { return false; }
}

bool InoutImpl::getInputTermination() const
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
  
  cycle.open(tr->getDevice());
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
  else                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
        
  readInputTermination = readInputTermination&(1<<internal_id);
  readInputTermination = readInputTermination>>internal_id;
  
  if (readInputTermination) { return true; }
  else                      { return false; }
}

void InoutImpl::setInputTermination(bool val)
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
  
  cycle.open(tr->getDevice());
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
  InputTermination(val);
}

bool InoutImpl::getOutputEnableAvailable() const
{
  return io_oe_available;
}

bool InoutImpl::getSpecialPurposeOutAvailable() const
{
  return io_spec_out_available;
}

bool InoutImpl::getInputTerminationAvailable() const
{
  return io_term_available;
}

bool InoutImpl::getSpecialPurposeInAvailable() const
{
  return io_spec_in_available;
}

bool InoutImpl::getSpecialPurposeOut() const
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
  
  cycle.open(tr->getDevice());
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
  else                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
        
  readSpecialPurposeOut = readSpecialPurposeOut&(1<<internal_id);
  readSpecialPurposeOut = readSpecialPurposeOut>>internal_id;
  
  if (readSpecialPurposeOut) { return true; }
  else                       { return false; }
}

void InoutImpl::setSpecialPurposeOut(bool val)
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
  
  cycle.open(tr->getDevice());
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
  SpecialPurposeOut(val);
}

bool InoutImpl::getSpecialPurposeIn() const
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
  
  cycle.open(tr->getDevice());
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
  else                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
        
  readSpecialPurposeIn = readSpecialPurposeIn&(1<<internal_id);
  readSpecialPurposeIn = readSpecialPurposeIn>>internal_id;
  
  if (readSpecialPurposeIn) { return true; }
  else                      { return false; }
}

void InoutImpl::setSpecialPurposeIn(bool val)
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
  
  cycle.open(tr->getDevice());
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
  SpecialPurposeIn(val);
}

int InoutImpl::probe(TimingReceiver* tr, TimingReceiver::ActionSinks& actionSinks, TimingReceiver::EventSources& eventSources)
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
  tr->getDevice().sdb_find_by_identity(IO_CONTROL_VENDOR_ID,     IO_CONTROL_PRODUCT_ID,     ioctl);
  tr->getDevice().sdb_find_by_identity(ECA_TLU_SDB_VENDOR_ID,    ECA_TLU_SDB_DEVICE_ID,     tlus);
  tr->getDevice().sdb_find_by_identity(IO_SER_CLK_GEN_VENDOR_ID, IO_SER_CLK_GEN_PRODUCT_ID, clkgen);
  
  if (ioctl.size() != 1 || tlus.size() != 1 || clkgen.size() != 1) return -1;
  eb_address_t ioctl_address = ioctl[0].sdb_component.addr_first;
  eb_address_t tlu = tlus[0].sdb_component.addr_first;
  eb_address_t clkgen_address = clkgen[0].sdb_component.addr_first;
  
  /* Get number of IOs */
  cycle.open(tr->getDevice());
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
    cycle.open(tr->getDevice());
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
  
  /* Create an action sink for each IO */
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
    Glib::ustring IOName = cIOName;
    
    /* Create the IO controller object */
    InoutImpl::ConstructorType impl_args = { 
      tr, channel, internal_id, special, logic_level, oe_available, 
      term_available, spec_out_available, spec_in_available, ioctl_address, clkgen_address };
    Glib::RefPtr<InoutImpl> impl(new InoutImpl(impl_args));

    unsigned eca_channel = 0; // ECA channel 0 is always for IO
    TimingReceiver::SinkKey key_in (eca_channel, eca_in);  // order: gpio_inout, gpio_in,  lvds_inout, lvds_in
    TimingReceiver::SinkKey key_out(eca_channel, eca_out); // order: gpio_inout, gpio_out, lvds_inout, lvds_out
    
    Glib::ustring input_path  = tr->getObjectPath() + "/inputs/"  + IOName;
    Glib::ustring output_path = tr->getObjectPath() + "/outputs/" + IOName;
    sigc::slot<void> nill;
    
    /* Add sinks depending on their direction */
    switch(direction)
    {
      case IO_CFG_FIELD_DIR_OUTPUT:
      {
        Output::ConstructorType out_args = { IOName, output_path, "", tr, eca_channel, eca_out, impl, nill };
        actionSinks[key_out] = Output::create(out_args);
        ++eca_out;
        break;
      }
      case IO_CFG_FIELD_DIR_INPUT:
      {
        Input::ConstructorType  in_args  = { IOName, input_path,  "", tr, tlu,         eca_in,  impl, nill };
        eventSources[key_in] = Input::create(in_args);
        ++eca_in;
        break;
      }
      case IO_CFG_FIELD_DIR_INOUT:
      {
        Output::ConstructorType out_args = { IOName, output_path, input_path,  tr, eca_channel, eca_out, impl, nill };
        Input::ConstructorType  in_args  = { IOName, input_path,  output_path, tr, tlu,         eca_in,  impl, nill };
        actionSinks[key_out] = Output::create(out_args);
        eventSources[key_in] = Input::create(in_args);
        ++eca_out;
        ++eca_in;
        break;
      }
      default:
      {
        clog << kLogErr << "Found IO with unknown direction!" << std::endl;
        return -1;
        break;
      }
    }
  }
  
  /* Done */
  return 0;
}

bool InoutImpl::getBuTiSMultiplexer() const
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
  
  cycle.open(tr->getDevice());
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
  else                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
        
  readBuTiSMultiplexer = readBuTiSMultiplexer&(1<<internal_id);
  readBuTiSMultiplexer = readBuTiSMultiplexer>>internal_id;
  
  if (readBuTiSMultiplexer) { return true; }
  else                      { return false; }
}

void InoutImpl::setBuTiSMultiplexer(bool val)
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
  
  cycle.open(tr->getDevice());
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
  BuTiSMultiplexer(val);
}

bool InoutImpl::getPPSMultiplexer() const
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
  
  cycle.open(tr->getDevice());
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
  else                        { throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
        
  readPPSMultiplexer = readPPSMultiplexer&(1<<internal_id);
  readPPSMultiplexer = readPPSMultiplexer>>internal_id;
  
  if (readPPSMultiplexer) { return true; }
  else                      { return false; }
}

void InoutImpl::setPPSMultiplexer(bool val)
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
  
  cycle.open(tr->getDevice());
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
  PPSMultiplexer(val);
}

bool InoutImpl::StartClock(double high_phase, double low_phase, guint64 phase_offset) { return ConfigureClock(high_phase, low_phase, phase_offset); }
bool InoutImpl::StopClock() { return ConfigureClock(0.0, 0.0, 0); }
bool InoutImpl::ConfigureClock(double high_phase, double low_phase, guint64 phase_offset)
{
  s_SerClkGenControl control;
  etherbone::Cycle cycle;
  
  /* Check if available */
  switch (io_channel) 
  {
    case IO_CFG_CHANNEL_LVDS:
    {
      break;
    }
    default: 
    { 
      throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "Clock generator is only available for LVDS outputs!");
      return false;
    }
  }
  
  /* Calculate values */
  CalcClockParameters(high_phase, low_phase, phase_offset, &control);
  
  /* Write values to clock generator */
  cycle.open(tr->getDevice());
  cycle.write(io_ser_clk_gen_addr+eSCK_selr,      EB_DATA32, io_index);
  cycle.write(io_ser_clk_gen_addr+eSCK_perr,      EB_DATA32, control.period_integer);
  cycle.write(io_ser_clk_gen_addr+eSCK_perhir,    EB_DATA32, control.period_high);
  cycle.write(io_ser_clk_gen_addr+eSCK_fracr,     EB_DATA32, control.period_fraction);
  cycle.write(io_ser_clk_gen_addr+eSCK_normmaskr, EB_DATA32, control.bit_pattern_normal);
  cycle.write(io_ser_clk_gen_addr+eSCK_skipmaskr, EB_DATA32, control.bit_pattern_skip);
  cycle.write(io_ser_clk_gen_addr+eSCK_phofslr,   EB_DATA32, (uint32_t)(control.phase_offset));
  cycle.write(io_ser_clk_gen_addr+eSCK_phofshr,   EB_DATA32, (uint32_t)(control.phase_offset >> 32));
  cycle.close();
  
  if ((low_phase == 0.0) && (high_phase == 0.0) && (phase_offset == 0)) { return false; }
  else                                                                  { return true; }
}

Glib::ustring InoutImpl::getLogicLevelOut() const { return getLogicLevel(); }
Glib::ustring InoutImpl::getLogicLevelIn() const { return getLogicLevel(); }
Glib::ustring InoutImpl::getLogicLevel() const
{
  Glib::ustring IOLogicLevel;
  
  switch(io_logic_level)
  {
    case IO_LOGIC_LEVEL_TTL:   { IOLogicLevel = "TTL";   break; }
    case IO_LOGIC_LEVEL_LVTTL: { IOLogicLevel = "LVTTL"; break; }
    case IO_LOGIC_LEVEL_LVDS:  { IOLogicLevel = "LVDS";  break; }
    case IO_LOGIC_LEVEL_NIM:   { IOLogicLevel = "NIM";   break; }
    case IO_LOGIC_LEVEL_CMOS:  { IOLogicLevel = "CMOS";  break; }
    default:                   { IOLogicLevel = "?";     break; }
  }
  
  return IOLogicLevel;
}

Glib::ustring InoutImpl::getTypeOut() const { return getType(); }
Glib::ustring InoutImpl::getTypeIn() const { return getType(); }
Glib::ustring InoutImpl::getType() const
{
  Glib::ustring IOType;
  
  switch(io_channel)
  {
    case IO_CFG_CHANNEL_GPIO: { IOType = "8ns (GPIO)"; break; }
    case IO_CFG_CHANNEL_LVDS: { IOType = "1ns (LVDS)"; break; }
    default: throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel type unknown!");
  }
  
  return IOType;
}

guint64 InoutImpl::getResolution() const
{
  switch (io_channel) {
  case IO_CFG_CHANNEL_GPIO: return 8;
  case IO_CFG_CHANNEL_LVDS: return 1;
  default: throw G10::BDus::Error(G10::BDus::Error::INVALID_ARGS, "IO channel resolution unknown!");
  }
}

} /* namespace saftlib */
