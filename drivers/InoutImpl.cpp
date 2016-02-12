#define ETHERBONE_THROWS 1

#include "TimingReceiver.h"
#include "InoutImpl.h"
#include "Output.h"
#include "Inoutput.h"
#include "Input.h"
#include "eca_regs.h"
#include "io_control_regs.h"
#include "src/clog.h"

namespace saftlib {

InoutImpl::InoutImpl(ConstructorType args)
 : ActionSink(args.dev, args.io_channel),
   io_channel(args.io_channel), io_index(args.io_index), io_delay(args.io_delay), io_logic_level(args.io_logic_level),
   io_oe_available(args.io_oe_available), io_term_available(args.io_term_available), io_spec_available(args.io_spec_available),
   io_control_addr(args.io_control_addr)
{
}

Glib::ustring InoutImpl::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards, bool on)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented");
}

void InoutImpl::WriteOutput(bool value)
{
  etherbone::Cycle cycle;
  eb_data_t writeOutput;
  
  ownerOnly();
  
  cycle.open(dev->getDevice());
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
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "IO channel unknown!");
  }
  cycle.close();
}

bool InoutImpl::ReadOutput()
{
  etherbone::Cycle cycle;
  eb_data_t readOutput;
  
  cycle.open(dev->getDevice());
  if (io_channel == IO_CFG_CHANNEL_GPIO)      { cycle.read(io_control_addr+eSet_GPIO_Out_Begin+(io_index*4), EB_DATA32, &readOutput); }
  else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eSet_LVDS_Out_Begin+(io_index*4), EB_DATA32, &readOutput); }
  else                                        { throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "IO channel unknown!"); }
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
  
  cycle.open(dev->getDevice());
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
  else                        { throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
  
  readOutputEnable = readOutputEnable&(1<<internal_id);
  readOutputEnable = readOutputEnable>>internal_id;
  
  if(readOutputEnable) { return true; }
  else                 { return false; }
}

void InoutImpl::setOutputEnable(bool val)
{
  unsigned access_position = 0;
  unsigned internal_id = io_index;
  etherbone::Cycle cycle;
    
  ownerOnly();
  
  /* Calculate access position (32bit access to 64bit register)*/
  if (io_index>31)
  { 
    internal_id = io_index-31; 
    access_position = 1;
  }
  
  cycle.open(dev->getDevice());
  if (io_channel == IO_CFG_CHANNEL_GPIO)
  {
    if (val)
    {
      if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Oe_Set_low,  EB_DATA32, (1<<internal_id)); }
      else                      { cycle.write(io_control_addr+eGPIO_Oe_Set_high, EB_DATA32, (1<<internal_id)); }
    }
    else
    {
      if (access_position == 0) { cycle.write(io_control_addr+eGPIO_Oe_Reset_low,  EB_DATA32, (1<<internal_id)); }
      else                      { cycle.write(io_control_addr+eGPIO_Oe_Reset_high, EB_DATA32, (1<<internal_id)); }
    }
  }
  else if (io_channel == IO_CFG_CHANNEL_LVDS)
  {
    if (val)
    {
      if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Oe_Set_low,  EB_DATA32, (1<<internal_id)); }
      else                      { cycle.write(io_control_addr+eLVDS_Oe_Set_high, EB_DATA32, (1<<internal_id)); }
    }
    else
    {
      if (access_position == 0) { cycle.write(io_control_addr+eLVDS_Oe_Reset_low,  EB_DATA32, (1<<internal_id)); }
      else                      { cycle.write(io_control_addr+eLVDS_Oe_Reset_high, EB_DATA32, (1<<internal_id)); }
    }
  }
  cycle.close();
}

guint64 InoutImpl::getResolution() const
{
  return 0;
}

guint32 InoutImpl::getEventBits() const
{
  return 0;
}

bool InoutImpl::getEventEnable() const
{
  return false;
}

guint64 InoutImpl::getEventPrefix() const
{
  return 0;
}

void InoutImpl::setEventEnable(bool val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented");
}

void InoutImpl::setEventPrefix(guint64 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented");
}

bool InoutImpl::ReadInput()
{
  etherbone::Cycle cycle;
  eb_data_t readInput;
  
  cycle.open(dev->getDevice());
  if      (io_channel == IO_CFG_CHANNEL_GPIO) { cycle.read(io_control_addr+eGet_GPIO_In_Begin+(io_index*4), EB_DATA32, &readInput); }
  else if (io_channel == IO_CFG_CHANNEL_LVDS) { cycle.read(io_control_addr+eGet_LVDS_In_Begin+(io_index*4), EB_DATA32, &readInput); }
  else                                        { throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
  
  if (readInput) { return true; }
  else           { return false; }
}

guint32 InoutImpl::getStableTime() const
{
  return 0;
}

bool InoutImpl::getInputTermination() const
{
  unsigned access_position = 0;
  unsigned internal_id = 0;
  eb_data_t readInputTermination;
  etherbone::Cycle cycle;
  
  /* Calculate access position (32bit access to 64bit register)*/
  if (io_index>31)
  { 
    internal_id = io_index-31; 
    access_position = 1;
  }
  
  cycle.open(dev->getDevice());
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
  else                        { throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "IO channel unknown!"); }
  cycle.close();
        
  readInputTermination = readInputTermination&(1<<internal_id);
  readInputTermination = readInputTermination>>internal_id;
  
  if (readInputTermination) { return true; }
  else                      { return false; }
}

void InoutImpl::setStableTime(guint32 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented");
}

void InoutImpl::setInputTermination(bool val)
{
  unsigned access_position = 0;
  unsigned internal_id = 0;
  etherbone::Cycle cycle;
    
  ownerOnly();
  
  /* Calculate access position (32bit access to 64bit register)*/
  if (io_index>31)
  { 
    internal_id = io_index-31; 
    access_position = 1;
  }
  
  cycle.open(dev->getDevice());
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
}

bool InoutImpl::getOutputEnableAvailable() const
{
  return io_oe_available;
}

bool InoutImpl::getSpecialPurposeOutAvailable() const
{
  return io_spec_available;
}

bool InoutImpl::getInputTerminationAvailable() const
{
  return io_term_available;
}

bool InoutImpl::getSpecialPurposeInAvailable() const
{
  return io_spec_available;
}

int InoutImpl::probe(TimingReceiver* tr, std::map< Glib::ustring, Glib::RefPtr<ActionSink> >& actionSinks)
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
  std::vector<sdb_device> ioctl;
  
  /* Find IO control module */
  tr->getDevice().sdb_find_by_identity(GSI_VENDOR_ID, 0x10c05791, ioctl);
  eb_address_t ioctl_address = ioctl[0].sdb_component.addr_first;
  
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
    io_table_addr = eIO_Map_Table_Begin + IO_DEVICE_NAME_BYTES + io_table_iterator*4;
    
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
      s_aIOCONTROL_SetupField[io_table_entries_id].uDelay         = (unsigned char) ((io_table_data_raw&0xff000000)>>24);
      s_aIOCONTROL_SetupField[io_table_entries_id].uInternalID    = (unsigned char) ((io_table_data_raw&0x00ff0000)>>16);
      s_aIOCONTROL_SetupField[io_table_entries_id].uIOCfgSpace    = (unsigned char) ((io_table_data_raw&0x0000ff00)>>8);
      s_aIOCONTROL_SetupField[io_table_entries_id].uLogicLevelRes = (unsigned char) ((io_table_data_raw&0x000000ff));
      io_table_entry_iterator = 0; 
      io_table_entries_id ++;
    }
  }
  
  /* Create an action sink for each IO */
  for (io_table_iterator = 0; io_table_iterator < (io_GPIOTotal + io_LVDSTotal); io_table_iterator++)
  {
    /* Helpers */
    char * cIOName;
    unsigned direction     = 0;
    unsigned internal_id   = 0;
    unsigned channel       = 0;
    unsigned delay         = 0;
    unsigned logic_level   = 0;
    bool oe_available   = false;
    bool term_available = false;
    bool spec_available = false;
    
    /* Get properties */
    direction   = (s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_DIR_MASK) >> IO_CFG_FIELD_DIR_SHIFT;
    internal_id = (s_aIOCONTROL_SetupField[io_table_iterator].uInternalID);
    channel     = (s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_INFO_CHAN_MASK) >> IO_CFG_FIELD_INFO_CHAN_SHIFT;
    delay       = (s_aIOCONTROL_SetupField[io_table_iterator].uDelay);
    logic_level = (s_aIOCONTROL_SetupField[io_table_iterator].uLogicLevelRes&IO_LOGIC_RES_FIELD_LL_MASK) >> IO_LOGIC_RES_FIELD_LL_SHIFT;
    
    /* Get available options */
    if ((s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_OE_MASK) >> IO_CFG_FIELD_OE_SHIFT)     { oe_available = true; }
    else                                                                                                            { oe_available = false; }
    if ((s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_TERM_MASK) >> IO_CFG_FIELD_TERM_SHIFT) { term_available = true; }
    else                                                                                                            { term_available = false; }
    if ((s_aIOCONTROL_SetupField[io_table_iterator].uIOCfgSpace&IO_CFG_FIELD_SPEC_MASK) >> IO_CFG_FIELD_SPEC_SHIFT) { spec_available = true; }
    else                                                                                                            { spec_available = false; }
    
    /* Get IO name */
    cIOName = s_aIOCONTROL_SetupField[io_table_iterator].uName;
    Glib::ustring IOName = cIOName;
    
    /* Add sinks depending on their direction */
    switch(direction)
    {
      case IO_CFG_FIELD_DIR_OUTPUT:
      {
        Output::ConstructorType args = { tr, channel, internal_id, delay, logic_level, oe_available, term_available, spec_available, ioctl_address };
        actionSinks[IOName] = Output::create(tr->getObjectPath() + "/outputs/" + IOName, args);
        break;
      }
      case IO_CFG_FIELD_DIR_INPUT:
      {
        Input::ConstructorType args = { tr, channel, internal_id, delay, logic_level, oe_available, term_available, spec_available, ioctl_address };
        actionSinks[IOName] = Input::create(tr->getObjectPath() + "/inputs/" + IOName, args);
        break;
      }
      case IO_CFG_FIELD_DIR_INOUT:
      {
        Inoutput::ConstructorType args = { tr, channel, internal_id, delay, logic_level, oe_available, term_available, spec_available, ioctl_address };
        actionSinks[IOName] = Inoutput::create(tr->getObjectPath() + "/inoutputs/" + IOName, args);
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

Glib::ustring InoutImpl::getLogicLevelOut() const { return getLogicLevel(); }
Glib::ustring InoutImpl::getLogicLevelIn() const { return getLogicLevel(); }
Glib::ustring InoutImpl::getLogicLevel() const
{
  Glib::ustring IOLogicLevel;
  
  switch(io_logic_level)
  {
    case IO_LOGIC_LEVEL_TTL:   { IOLogicLevel = "TTL"; break; }
    case IO_LOGIC_LEVEL_LVTTL: { IOLogicLevel = "LVTTL"; break; }
    case IO_LOGIC_LEVEL_LVDS:  { IOLogicLevel = "LVDS"; break; }
    case IO_LOGIC_LEVEL_NIM:   { IOLogicLevel = "NIM"; break; }
    default:                   { IOLogicLevel = "?"; break; }
  }
  
  return IOLogicLevel;
}

} /* namespace saftlib */
