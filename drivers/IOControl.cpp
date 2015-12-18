#define ETHERBONE_THROWS 1

#include "clog.h"
#include "IOControl.h"
#include "TimingReceiver.h"
#include "io_control_regs.h"
#include "RegisteredObject.h"

namespace saftlib 
{
  /* Constructor */
  /* ==================================================================================================== */
  IOControl::IOControl(ConstructorType args)
  {
    /* Helpers */
    guint32 gpio_count_reg          = 0;
    guint32 lvds_count_reg          = 0;
    guint32 io_version_reg          = 0;
    guint32 io_fixed_reg            = 0;
    guint32 io_table_entries_id     = 0;
    guint32 io_table_iterator       = 0;
    guint32 io_table_entry_iterator = 0;
    guint32 io_table_data_raw       = 0;
    guint32 io_table_addr           = 0;
    guint32 device_name_part[IO_DEVICE_NAME_BYTES/4];
    
    /* Initialize parameters */
    dev = args.dev;
    DeviceAddress = args.DeviceAddress;
    
    /* Get the IO count */
    gpio_count_reg = GetParameter(eGPIO_Info);
    lvds_count_reg = GetParameter(eLVDS_Info);
    io_version_reg = GetParameter(eIO_Version);
    io_fixed_reg   = GetParameter(eFIXED_Info);
    
    /* Extract information */
    GPIOInputs    = (gpio_count_reg&IO_INFO_IN_COUNT_MASK)    >> IO_INFO_IN_SHIFT;
    GPIOInoutputs = (gpio_count_reg&IO_INFO_INOUT_COUNT_MASK) >> IO_INFO_INOUT_SHIFT;
    GPIOOutputs   = (gpio_count_reg&IO_INFO_OUT_COUNT_MASK)   >> IO_INFO_OUT_SHIFT;
    GPIOTotal     = (gpio_count_reg&IO_INFO_TOTAL_COUNT_MASK) >> IO_INFO_TOTAL_SHIFT;
    LVDSInputs    = (lvds_count_reg&IO_INFO_IN_COUNT_MASK)    >> IO_INFO_IN_SHIFT;
    LVDSInoutputs = (lvds_count_reg&IO_INFO_INOUT_COUNT_MASK) >> IO_INFO_INOUT_SHIFT;
    LVDSOutputs   = (lvds_count_reg&IO_INFO_OUT_COUNT_MASK)   >> IO_INFO_OUT_SHIFT;
    LVDSTotal     = (lvds_count_reg&IO_INFO_TOTAL_COUNT_MASK) >> IO_INFO_TOTAL_SHIFT;
    FIXEDTotal    = io_fixed_reg;
    IOVersion     = io_version_reg;
    
    /* Get the device name */
    for (io_table_iterator = 0; io_table_iterator < IO_DEVICE_NAME_BYTES/4; io_table_iterator++)
    {
      /* Read memory region */
      device_name_part[io_table_iterator] = GetParameter(eIO_Map_Table_Begin+(io_table_iterator*4));
      /* Convert uint32 to 4x unit8 */
      c_aDeviceName[(io_table_iterator*4)+0] = char ((device_name_part[io_table_iterator]&0xff000000)>>24);
      c_aDeviceName[(io_table_iterator*4)+1] = char ((device_name_part[io_table_iterator]&0x00ff0000)>>16);
      c_aDeviceName[(io_table_iterator*4)+2] = char ((device_name_part[io_table_iterator]&0x0000ff00)>>8);
      c_aDeviceName[(io_table_iterator*4)+3] = char ((device_name_part[io_table_iterator]&0x000000ff));
    }
    DeviceName = c_aDeviceName;
    
#if IO_DEBUG_MODE
    clog << kLogDebug << "IOControl device/table build name is: <" << c_aDeviceName << ">" << std::endl;
#endif
    
    /* Read IO information */
    for (io_table_iterator = 0; io_table_iterator < (GPIOTotal*4 + LVDSTotal*4 + FIXEDTotal*4); io_table_iterator++)
    {
      /* Read memory region */
      io_table_addr = eIO_Map_Table_Begin + IO_DEVICE_NAME_BYTES + io_table_iterator*4;
      io_table_data_raw = GetParameter(io_table_addr);
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
    
#if IO_DEBUG_MODE
    clog << kLogDebug << "IOControl module loaded!"  << std::endl;
#endif
  }
  
  /* Destructor */
  /* ==================================================================================================== */
  IOControl::~IOControl()
  {
#if IO_DEBUG_MODE
    clog << kLogDebug << "IOControl module unloaded!" << std::endl;
#endif
  }
  
  Glib::RefPtr<IOControl> IOControl::create(const Glib::ustring& objectPath, ConstructorType args)
  {
    return RegisteredObject<IOControl>::create(objectPath, args);
  }
  
  void IOControl::SetParameter(guint32 param, guint32 value)
  {
    /* Helpers */
    guint32 uRegisterOffset = param;
    etherbone::Cycle cycle;
    eb_data_t set_param = value;
    
    /* Open a new cycle and set the parameter */
    cycle.open(dev->getDevice());
    cycle.write(DeviceAddress+uRegisterOffset, EB_DATA32, set_param);
    cycle.close();
  }
  
  guint32 IOControl::GetParameter(guint32 param)
  { 
    /* Helpers */
    guint32 uRegisterOffset = param;
    etherbone::Cycle cycle;
    eb_data_t get_param = 0;
    
    /* Open a new cycle and get the parameter */
    cycle.open(dev->getDevice());
    cycle.read(DeviceAddress+uRegisterOffset, EB_DATA32, &get_param);
    cycle.close();
    
    /* Return parameter */
    return (get_param);
  }
  
  guint32 IOControl::CheckIoName(const Glib::ustring& name)
  {
    /* Helper */
    char * cIOName;
    Glib::ustring IOName;
    guint32 io_table_entry_iterator = 0;
    
    /* Search for the IO name */
    for (io_table_entry_iterator = 0; io_table_entry_iterator < GPIOTotal + LVDSTotal + FIXEDTotal; io_table_entry_iterator++)
    {
      /* Get name from table */
      cIOName = s_aIOCONTROL_SetupField[io_table_entry_iterator].uName;
      IOName = cIOName;
      
      /* Return number */
      if (IOName == name) { return (__IO_RETURN_SUCCESS); }
    }
    
    /* Not found */
    return (__IO_RETURN_FAILURE);
  }
  
  guint32 IOControl::GetIoTableParameterByNumber(guint32 number, guint32 param)
  {
    /* Helper */
    guint32 ret_param           = 0;
    guint32 reg_delay           = s_aIOCONTROL_SetupField[number].uDelay;
    guint32 reg_internal_id     = s_aIOCONTROL_SetupField[number].uInternalID;
    guint32 reg_cfg_field       = s_aIOCONTROL_SetupField[number].uIOCfgSpace;
    guint32 reg_logic_res_field = s_aIOCONTROL_SetupField[number].uLogicLevelRes;
    
    /* Switch depending on parameter */
    switch(param)
    {
      /* Extract property */
      case eDelay:        { ret_param = (reg_delay);                                                                       break; }
      case eInternalId:   { ret_param = (reg_internal_id);                                                                 break; }
      case eDirection:    { ret_param = (reg_cfg_field&IO_CFG_FIELD_DIR_MASK)             >> IO_CFG_FIELD_DIR_SHIFT;       break; }
      case eOutputEnable: { ret_param = (reg_cfg_field&IO_CFG_FIELD_OE_MASK)              >> IO_CFG_FIELD_OE_SHIFT;        break; }
      case eTermination:  { ret_param = (reg_cfg_field&IO_CFG_FIELD_TERM_MASK)            >> IO_CFG_FIELD_TERM_SHIFT;      break; }
      case eSpecial:      { ret_param = (reg_cfg_field&IO_CFG_FIELD_SPEC_MASK)            >> IO_CFG_FIELD_SPEC_SHIFT;      break; } 
      case eChannel:      { ret_param = (reg_cfg_field&IO_CFG_FIELD_INFO_CHAN_MASK)       >> IO_CFG_FIELD_INFO_CHAN_SHIFT; break; }
      case eLogicLevel:   { ret_param = (reg_logic_res_field&IO_LOGIC_RES_FIELD_LL_MASK)  >> IO_LOGIC_RES_FIELD_LL_SHIFT;  break; }
      case eReserved:     { ret_param = (reg_logic_res_field&IO_LOGIC_RES_FIELD_RES_MASK) >> IO_LOGIC_RES_FIELD_RES_SHIFT; break; }
      default:            { ret_param = IO_FIELD_PARAM_INVALID; }
    }
    
    /* Return the selected ret_param */
    return (ret_param);
  }
  
  guint32 IOControl::GetIoTableParameterByName(const Glib::ustring& name, guint32 param)
  {
    /* Helpers */
    guint32 ret_param = 0;
    guint32 table_number = 0;
    
    /* Get the table number */
    table_number = GetIoTableNumber(name);
    if (table_number == IO_FIELD_PARAM_INVALID) { return (__IO_RETURN_FAILURE); }
    
    /* Get the parameter */
    ret_param = GetIoTableParameterByNumber(table_number, param);
    return (ret_param);
  }
  
  Glib::ustring IOControl::GetIoTableName(guint32 number)
  {
    /* Helper */
    char * cIOName;
    cIOName = s_aIOCONTROL_SetupField[number].uName;
    Glib::ustring IOName = cIOName;
    
    /* Return name */
    return IOName;
  }
  
  guint32 IOControl::GetIoTableNumber(const Glib::ustring& name)
  {
    /* Helper */
    char * cIOName;
    Glib::ustring IOName;
    guint32 io_table_entry_iterator = 0;
    
    /* Search for the IO name */
    for (io_table_entry_iterator = 0; io_table_entry_iterator < GPIOTotal + LVDSTotal + FIXEDTotal; io_table_entry_iterator++)
    {
      /* Get name from table */
      cIOName = s_aIOCONTROL_SetupField[io_table_entry_iterator].uName;
      IOName = cIOName;
      
      /* Return number */
      if (IOName == name) { return io_table_entry_iterator; }
    }
    
    /* Not found */
    return (__IO_RETURN_FAILURE);
  }
  
  guint32 IOControl::GetInternalId(const Glib::ustring& name)
  {
    return (GetIoTableParameterByName(name, eInternalId));
  }
  
  guint32 IOControl::GetDirection(const Glib::ustring& name)
  {
    return (GetIoTableParameterByName(name, eDirection));
  }
  
  guint32 IOControl::GetChannel(const Glib::ustring& name)
  {
    return (GetIoTableParameterByName(name, eChannel));
  }
  
  guint32 IOControl::SetOe(const Glib::ustring& name)   { return (CommandWrapper(name, IO_OPERATION_SET, IO_TYPE_OE, NULL)); }
  guint32 IOControl::ResetOe(const Glib::ustring& name) { return (CommandWrapper(name, IO_OPERATION_RESET, IO_TYPE_OE, NULL)); }
  guint32 IOControl::GetOe(const Glib::ustring& name)
  { 
    guint32 value = 0;
    CommandWrapper(name, IO_OPERATION_GET, IO_TYPE_OE, &value);
    return (value);
  }

  guint32 IOControl::SetTerm(const Glib::ustring& name)   { return (CommandWrapper(name, IO_OPERATION_SET, IO_TYPE_TERM, NULL)); }
  guint32 IOControl::ResetTerm(const Glib::ustring& name) { return (CommandWrapper(name, IO_OPERATION_RESET, IO_TYPE_TERM, NULL)); }
  guint32 IOControl::GetTerm(const Glib::ustring& name)
  { 
    guint32 value = 0;
    CommandWrapper(name, IO_OPERATION_GET, IO_TYPE_TERM, &value);
    return (value);
  }
  
  guint32 IOControl::SetSpecIn(const Glib::ustring& name)   { return (CommandWrapper(name, IO_OPERATION_SET, IO_TYPE_SPEC_IN, NULL)); }
  guint32 IOControl::ResetSpecIn(const Glib::ustring& name) { return (CommandWrapper(name, IO_OPERATION_RESET, IO_TYPE_SPEC_IN, NULL)); }
  guint32 IOControl::GetSpecIn(const Glib::ustring& name)
  { 
    guint32 value = 0;
    CommandWrapper(name, IO_OPERATION_GET, IO_TYPE_SPEC_IN, &value);
    return (value);
  }
  
  guint32 IOControl::SetSpecOut(const Glib::ustring& name)   { return (CommandWrapper(name, IO_OPERATION_SET, IO_TYPE_SPEC_OUT, NULL)); }
  guint32 IOControl::ResetSpecOut(const Glib::ustring& name) { return (CommandWrapper(name, IO_OPERATION_RESET, IO_TYPE_SPEC_OUT, NULL)); }
  guint32 IOControl::GetSpecOut(const Glib::ustring& name)
  { 
    guint32 value = 0;
    CommandWrapper(name, IO_OPERATION_GET, IO_TYPE_SPEC_OUT, &value);
    return (value);
  }
  
  guint32 IOControl::SetMux(const Glib::ustring& name)   { return (CommandWrapper(name, IO_OPERATION_SET, IO_TYPE_MUX, NULL)); }
  guint32 IOControl::ResetMux(const Glib::ustring& name) { return (CommandWrapper(name, IO_OPERATION_RESET, IO_TYPE_MUX, NULL)); }
  guint32 IOControl::GetMux(const Glib::ustring& name)
  { 
    guint32 value = 0;
    CommandWrapper(name, IO_OPERATION_GET, IO_TYPE_MUX, &value);
    return (value);
  }
  
  guint32 IOControl::SetSel(const Glib::ustring& name)   { return (CommandWrapper(name, IO_OPERATION_SET, IO_TYPE_SEL, NULL)); }
  guint32 IOControl::ResetSel(const Glib::ustring& name) { return (CommandWrapper(name, IO_OPERATION_RESET, IO_TYPE_SEL, NULL)); }
  guint32 IOControl::GetSel(const Glib::ustring& name)
  { 
    guint32 value = 0;
    CommandWrapper(name, IO_OPERATION_GET, IO_TYPE_SEL, &value);
    return (value);
  }
  
  guint32 IOControl::GetOutput(const Glib::ustring& name) 
  { 
    /* Helpers */
    guint32 value = 0;
    guint32 channel = 0;
    guint32 direction = 0;
    guint32 internal_id = 0;
    
    /* Check if IO name really exists */
    if (CheckIoName(name)) { return (__IO_RETURN_FAILURE); }
    
    /* Get channel, direction and internal ID  */
    channel = GetChannel(name);
    direction = GetDirection(name);
    internal_id = GetInternalId(name);
    
    /* Plausibility check */
    if (internal_id < IO_MAX_IOS_PER_CHANNEL && channel < IO_MAX_VALID_CHANNELS && (direction == IO_CFG_FIELD_DIR_INOUT || direction == IO_CFG_FIELD_DIR_OUTPUT))
    {
      if (channel == IO_CFG_CHANNEL_GPIO) 
      { 
        value = GetParameter(eSet_GPIO_Out_Begin+(internal_id*4));
        /* GPIO correction */
        if (value) { value = 0xff; }
      }
      else if (channel == IO_CFG_CHANNEL_LVDS) { value = GetParameter(eSet_LVDS_Out_Begin+(internal_id*4)); }
      else { return (__IO_RETURN_FAILURE); }
    }
    
    /* Done */
    return (value);
  }
  
  guint32 IOControl::GetOutputCombined(const Glib::ustring& name)
  { 
    /* Helpers */
    guint32 value = 0;
    guint32 channel = 0;
    guint32 direction = 0;
    guint32 internal_id = 0;
    guint32 io_in_inout_offset = 0;
    
    /* Check if IO name really exists */
    if (CheckIoName(name)) { return (__IO_RETURN_FAILURE); }
    
    /* Get channel, direction and internal ID  */
    channel = GetChannel(name);
    direction = GetDirection(name);
    internal_id = GetInternalId(name);
    
    /* Plausibility check */
    if (internal_id < IO_MAX_IOS_PER_CHANNEL && channel < IO_MAX_VALID_CHANNELS && (direction == IO_CFG_FIELD_DIR_INOUT || direction == IO_CFG_FIELD_DIR_OUTPUT))
    {
      if (channel == IO_CFG_CHANNEL_GPIO)
      {
        io_in_inout_offset = getGPIOInoutputs() + getGPIOInputs();
        value = GetParameter(eGet_GPIO_In_Begin+(internal_id*4)+(io_in_inout_offset*4));
        /* GPIO correction */
        if (value) { value = 0xff; }
      }
      else if (channel == IO_CFG_CHANNEL_LVDS)
      { 
        io_in_inout_offset = getLVDSInoutputs() + getLVDSInputs();
        value = GetParameter(eGet_LVDS_In_Begin+(internal_id*4)+(io_in_inout_offset*4));
      }
      else { return (__IO_RETURN_FAILURE); }
    }
    
    /* Done */
    return (value);
  }
  
  guint32 IOControl::GetInput(const Glib::ustring& name)
  { 
    /* Helpers */
    guint32 value = 0;
    guint32 channel = 0;
    guint32 direction = 0;
    guint32 internal_id = 0;
    
    /* Check if IO name really exists */
    if (CheckIoName(name)) { return (__IO_RETURN_FAILURE); }
    
    /* Get channel, direction and internal ID  */
    channel = GetChannel(name);
    direction = GetDirection(name);
    internal_id = GetInternalId(name);
    
    /* Plausibility check */
    if (internal_id < IO_MAX_IOS_PER_CHANNEL && channel < IO_MAX_VALID_CHANNELS && (direction == IO_CFG_FIELD_DIR_INOUT || direction == IO_CFG_FIELD_DIR_INPUT))
    {
      if (channel == IO_CFG_CHANNEL_GPIO) 
      { 
        value = GetParameter(eGet_GPIO_In_Begin+(internal_id*4));
        /* GPIO correction */
        if (value) { value = 0xff; }
      }
      else if (channel == IO_CFG_CHANNEL_LVDS) { value = GetParameter(eGet_LVDS_In_Begin+(internal_id*4)); }
      else { return (__IO_RETURN_FAILURE); }
    }
    
    /* Done */
    return (value);
  }
  
  guint32 IOControl::DriveLow(const Glib::ustring& name)  { return (DriveWrapper(name, 0)); }
  guint32 IOControl::DriveHigh(const Glib::ustring& name) { return (DriveWrapper(name, 1)); }
    
  guint32 IOControl::CommandWrapper(const Glib::ustring& name, guint32 operation, guint32 type, guint32 * value)
  {
    /* Helpers */
    guint32 channel = 0;
    guint32 internal_id = 0;
    guint32 access_position = 0;
    
    /* Check if IO name really exists */
    if (CheckIoName(name)) { return (__IO_RETURN_FAILURE); }
    
    /* Get ID and channel */
    internal_id = GetInternalId(name);
    channel = GetChannel(name);
    
    /* Plausibility check */
    if(internal_id < IO_MAX_IOS_PER_CHANNEL && channel < IO_MAX_VALID_CHANNELS)
    {
      /* Calculate access position (32bit access to 64bit register)*/
      if (internal_id>31)
      { 
        internal_id = internal_id-31; 
        access_position = 1;
      }
      
      /* Check operation */
      if (operation == IO_OPERATION_SET)
      {
        /* Set Oe */
        if (type == IO_TYPE_OE)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Oe_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Oe_Set_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Oe_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Oe_Set_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Set Term */
        if (type == IO_TYPE_TERM)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Term_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Term_Set_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Term_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Term_Set_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Set Spec In */
        if (type == IO_TYPE_SPEC_IN)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Spec_In_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Spec_In_Set_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Spec_In_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Spec_In_Set_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
          
        /* Set Spec Out */
        if (type == IO_TYPE_SPEC_OUT)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Spec_Out_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Spec_Out_Set_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Spec_Out_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Spec_Out_Set_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Set Mux */
        if (type == IO_TYPE_MUX)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Mux_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Mux_Set_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Mux_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Mux_Set_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Set Sel */
        if (type == IO_TYPE_SEL)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Sel_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Sel_Set_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Sel_Set_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Sel_Set_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
      }
      else if (operation == IO_OPERATION_RESET)
      {
        /* Reset Oe */
        if (type == IO_TYPE_OE)
        {
              clog << kLogDebug << "Reset" << std::endl;
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Oe_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Oe_Reset_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Oe_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Oe_Reset_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Reset Term */
        if (type == IO_TYPE_TERM)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Term_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Term_Reset_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Term_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Term_Reset_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Reset Spec In */
        if (type == IO_TYPE_SPEC_IN)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Spec_In_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Spec_In_Reset_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Spec_In_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Spec_In_Reset_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Reset Spec Out */
        if (type == IO_TYPE_SPEC_OUT)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Spec_Out_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Spec_Out_Reset_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Spec_Out_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Spec_Out_Reset_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Reset Mux */
        if (type == IO_TYPE_MUX)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Mux_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Mux_Reset_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Mux_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Mux_Reset_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Reset Sel */
        if (type == IO_TYPE_SEL)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { SetParameter(eGPIO_Sel_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eGPIO_Sel_Reset_high, (1<<internal_id)); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { SetParameter(eLVDS_Sel_Reset_low,  (1<<internal_id)); }
            else                      { SetParameter(eLVDS_Sel_Reset_high, (1<<internal_id)); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
      }
      else if (operation == IO_OPERATION_GET)
      {
        /* Get Oe */
        if (type == IO_TYPE_OE)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { *value = GetParameter(eGPIO_Oe_Set_low); }
            else                      { *value = GetParameter(eGPIO_Oe_Set_high); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { *value = GetParameter(eLVDS_Oe_Set_low); }
            else                      { *value = GetParameter(eLVDS_Oe_Set_high); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Get Term */
        if (type == IO_TYPE_TERM)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { *value = GetParameter(eGPIO_Term_Set_low); }
            else                      { *value = GetParameter(eGPIO_Term_Set_high); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { *value = GetParameter(eLVDS_Term_Set_low); }
            else                      { *value = GetParameter(eLVDS_Term_Set_high); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Get Spec In */
        if (type == IO_TYPE_SPEC_IN)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { *value = GetParameter(eGPIO_Spec_In_Set_low); }
            else                      { *value = GetParameter(eGPIO_Spec_In_Set_high); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { *value = GetParameter(eLVDS_Spec_In_Set_low); }
            else                      { *value = GetParameter(eLVDS_Spec_In_Set_high); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Get Spec Out */
        if (type == IO_TYPE_SPEC_OUT)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { *value = GetParameter(eGPIO_Spec_Out_Set_low); }
            else                      { *value = GetParameter(eGPIO_Spec_Out_Set_high); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { *value = GetParameter(eLVDS_Spec_Out_Set_low); }
            else                      { *value = GetParameter(eLVDS_Spec_Out_Set_high); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Get Mux */
        if (type == IO_TYPE_MUX)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { *value = GetParameter(eGPIO_Mux_Set_low); }
            else                      { *value = GetParameter(eGPIO_Mux_Set_high); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { *value = GetParameter(eLVDS_Mux_Set_low); }
            else                      { *value = GetParameter(eLVDS_Mux_Set_high); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Get Sel */
        if (type == IO_TYPE_SEL)
        {
          if (channel == IO_CFG_CHANNEL_GPIO)
          {
            if (access_position == 0) { *value = GetParameter(eGPIO_Sel_Set_low); }
            else                      { *value = GetParameter(eGPIO_Sel_Set_high); }
          }
          else if (channel == IO_CFG_CHANNEL_LVDS)
          {
            if (access_position == 0) { *value = GetParameter(eLVDS_Sel_Set_low); }
            else                      { *value = GetParameter(eLVDS_Sel_Set_high); }
          }
          else { return (__IO_RETURN_FAILURE); }
        }
        
        /* Extract value and shift it to position 0 (msb) */
        *value = (*value & (1<<internal_id))>>internal_id;
      }
      else
      {
        return (__IO_RETURN_FAILURE);
      }
    }
    else
    {
      return (__IO_RETURN_FAILURE);
    }
    
    /* Done */
    return (__IO_RETURN_SUCCESS);
  }
      
  guint32 IOControl::DriveWrapper(const Glib::ustring& name, guint32 value)
  {
    /* Helpers */
    guint32 channel = 0;
    guint32 direction = 0;
    guint32 internal_id = 0;
    
    /* Check if IO name really exists */
    if (CheckIoName(name)) { return (__IO_RETURN_FAILURE); }
    
    /* Get channel, direction and internal ID  */
    channel = GetChannel(name);
    direction = GetDirection(name);
    internal_id = GetInternalId(name);
    
    /* Plausibility check */
    if (internal_id < IO_MAX_IOS_PER_CHANNEL && channel < IO_MAX_VALID_CHANNELS && (direction == IO_CFG_FIELD_DIR_INOUT || direction == IO_CFG_FIELD_DIR_OUTPUT))
    {
      if (channel == IO_CFG_CHANNEL_GPIO) 
      { 
        if (value == 1) { SetParameter(eSet_GPIO_Out_Begin+(internal_id*4), 0xff); }
        else            { SetParameter(eSet_GPIO_Out_Begin+(internal_id*4), 0x00); }
      }
      else if (channel == IO_CFG_CHANNEL_LVDS)
      { 
        if (value == 1) { SetParameter(eSet_LVDS_Out_Begin+(internal_id*4), 0xff); }
        else            { SetParameter(eSet_LVDS_Out_Begin+(internal_id*4), 0x00); }
      }
      else { return (__IO_RETURN_FAILURE); }
    }
    
    /* Done */
    return (__IO_RETURN_SUCCESS);
  }
      
  guint32 IOControl::getGPIOInputs()       const { return (GPIOInputs); }
  guint32 IOControl::getGPIOInoutputs()    const { return (GPIOInoutputs); }
  guint32 IOControl::getGPIOOutputs()      const { return (GPIOOutputs); }
  guint32 IOControl::getGPIOTotal()        const { return (GPIOTotal); }
  guint32 IOControl::getLVDSInputs()       const { return (LVDSInputs); }
  guint32 IOControl::getLVDSInoutputs()    const { return (LVDSInoutputs); }
  guint32 IOControl::getLVDSOutputs()      const { return (LVDSOutputs); }
  guint32 IOControl::getLVDSTotal()        const { return (LVDSTotal); }
  guint32 IOControl::getFIXEDTotal()       const { return (FIXEDTotal); }
  guint32 IOControl::getIOVersion()        const { return (IOVersion); }
  Glib::ustring IOControl::getDeviceName() const { return (DeviceName); }
  
} /* namespace saftlib */


