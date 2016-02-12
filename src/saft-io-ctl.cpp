/* Synopsis */
/* ==================================================================================================== */
/* IO control application */

/* Defines */
/* ==================================================================================================== */
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

/* Includes */
/* ==================================================================================================== */
#include <iostream>
#include <iomanip>
#include <giomm.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"
#include "interfaces/iDevice.h"
#include "interfaces/IOControl.h"
#include "interfaces/Output.h"
#include "interfaces/iOutputActionSink.h"
#include "interfaces/iActionSink.h"
#include "interfaces/Input.h"

#include "drivers/io_control_regs.h"

/* Namespace */
/* ==================================================================================================== */
using namespace saftlib;
using namespace std;

/* Global */
/* ==================================================================================================== */
static const char *program    = NULL; /* Name of the application */
static const char *deviceName = NULL; /* Name of the device */
static const char *ioName     = NULL; /* Name of the io */

/* Prototypes */
/* ==================================================================================================== */
static void io_help        (void);
static int  io_setup       (int io_oe, int io_term, int io_spec_out, int io_spec_in, int io_mux, int io_drive, 
                            bool set_oe, bool set_term, bool set_spec_out, bool set_spec_in, bool set_mux, bool set_drive,
                            bool verbose_mode);
static int  io_print_table (bool verbose_mode);

/* Function io_help() */
/* ==================================================================================================== */
static void io_help (void)
{
  /* Print arguments and options */
  std::cout << "IO-CTL for SAFTlib " << __IO_CTL_VERSION << std::endl;
  std::cout << "Usage: " << program << " <unique device name> <io name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -o 0/1: Toggle output enable" << std::endl;
  std::cout << "  -t 0/1: Toggle termination/resistor" << std::endl;
  std::cout << "  -p 0/1: Toggle special (output) functionality" << std::endl;
  std::cout << "  -e 0/1: Toggle special (input) functionality" << std::endl;
  std::cout << "  -m 0/1: Toggle output multiplexer (ECA/IOC/ClkGen or BuTiS t0 + TS)" << std::endl;
  std::cout << "  -d 0/1: Toggle output value" << std::endl;
  std::cout << "  -h: Print help (this message)" << std::endl;
  std::cout << "  -v: Switch to verbose mode" << std::endl;
  std::cout << "  -i: List every IO and it's capability" << std::endl;
  std::cout << std::endl;
  std::cout << "Example: " << program << " exploder5a_123t " << "IO1 " << "-o 1 -t 0 -d 1" << std::endl;
  std::cout << "  This will enable the output and disable the (input) termination/resistor and drive the output high" << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <csco-tg@gsi.de>" << std::endl;
  std::cout << "Licensed under the GPLv3" << std::endl;
  std::cout << std::endl;
}

///* Function io_setup() */
///* ==================================================================================================== */
//static int  io_setup (int io_oe, int io_term, int io_spec, int io_mux, int io_drive, 
//                      bool set_oe, bool set_term, bool set_spec, bool set_mux, bool set_drive,
//                      bool verbose_mode)
//{
//  /* Helpers */
//  int  io_channel       = 0;
//  int  io_direction     = 0;
//  int  io_oe_cfg        = 0;
//  int  io_term_cfg      = 0;
//  int  io_spec_cfg      = 0;
//  int  io_oe_status     = 0;
//  int  io_term_status   = 0;
//  int  io_spec_status   = 0;
//  int  io_mux_status    = 0;
//  int  io_current_state = 0;
//  int  return_value     = 0;
//  bool io_set           = false; /* IO set or get */
//  
//  /* Check if there is at least one parameter to set */
//  io_set = set_oe | set_term | set_spec | set_mux | set_drive;
//  
//  /* Display information */
//  if (io_set) { std::cout << "Checking configuration feasibility..." << std::endl; }
//  else        { std::cout << "Checking configuration..." << std::endl; }
//  
//  /* Initialize SAFTLib components */
//  Gio::init();
//  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
//  
//  /* Perform plausibility check and apply changes */
//  try
//  {
//    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
//    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
//    map<Glib::ustring, Glib::ustring> ios_proxy = receiver->getInterfaces()["IOControl"];
//    if (ios_proxy.size() != 1)
//    {
//      std::cout << "Interface(s) found: " << ios_proxy.size() << std::endl;
//      std::cout << "Expecting 1 interface only!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    Glib::RefPtr<IOControl_Proxy> ioctrl = IOControl_Proxy::create(ios_proxy.begin()->second);
//    
//    /* Check if IO name really exists */
//    if (ioctrl->CheckIoName(ioName))
//    { 
//      std::cout << "There is no IO with the name "<< ioName << "!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    
//    /* Collect IO information for plausibility checks */
//    io_channel     = ioctrl->GetIoTableParameterByName(ioName, eChannel);
//    io_direction   = ioctrl->GetIoTableParameterByName(ioName, eDirection);
//    io_oe_cfg      = ioctrl->GetIoTableParameterByName(ioName, eOutputEnable);
//    io_term_cfg    = ioctrl->GetIoTableParameterByName(ioName, eTermination);
//    io_spec_cfg    = ioctrl->GetIoTableParameterByName(ioName, eSpecial);
//    
//    /* Plausibility checks for setup */
//    if (io_channel == IO_CFG_CHANNEL_FIXED) { std::cout << "There is no setup or information for " << ioName << "!" << std::endl; }
//    
//    /* Plausibility checks for setup */
//    if (set_oe == true && io_oe_cfg == 0)
//    {
//      std::cout << "Output enable setup is not available for "<< ioName << "!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    if (set_term == true && io_term_cfg == 0)
//    {
//      std::cout << "Termination setup is not available for "<< ioName << "!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    if (set_spec == true && io_spec_cfg == 0)
//    {
//      std::cout << "Special setup is not available for "<< ioName << "!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    if (set_mux == true && io_direction == IO_CFG_FIELD_DIR_INPUT)
//    {
//      std::cout << "BuTiS t0 + TS multiplexer is not available for "<< ioName << "!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    if (set_drive == true && io_direction == IO_CFG_FIELD_DIR_INPUT)
//    {
//      std::cout << "There is not driver option for "<< ioName << "!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    
//    /* Set/get IO configuration */
//    if (io_set)
//    {
//      /* Set OE */
//      if (set_oe)
//      { 
//        if (io_oe) { return_value = ioctrl->SetOe(ioName); }
//        else       { return_value = ioctrl->ResetOe(ioName); }
//      }
//      /* Set TERM */
//      if (set_term)
//      { 
//        if (io_term) { return_value = ioctrl->SetTerm(ioName); }
//        else         { return_value = ioctrl->ResetTerm(ioName); }
//      }
//      /* Set SPEC */
//      if (set_spec)
//      { 
//        /* Input */
//        if (io_spec) { return_value = ioctrl->SetSpecIn(ioName); }
//        else         { return_value = ioctrl->ResetSpecIn(ioName); }
//        
//        /* Check return value because we are setting two options here */
//        if (return_value == __IO_RETURN_FAILURE) { return (return_value); }
//        
//        /* Output */
//        if (io_spec) { return_value = ioctrl->SetSpecOut(ioName); }
//        else         { return_value = ioctrl->ResetSpecOut(ioName); }
//      }
//      /* Set MUX */
//      if (set_mux)
//      { 
//        if (io_mux) { return_value = ioctrl->SetMux(ioName); }
//        else        { return_value = ioctrl->ResetMux(ioName); }
//      }
//      /* Drive output */
//      if (set_drive)
//      { 
//        if (io_drive) { return_value = ioctrl->DriveHigh(ioName); }
//        else          { return_value = ioctrl->DriveLow(ioName); }
//      }
//      /* Setup done */
//      std::cout << "Setup for " << ioName << " done..." << std::endl;
//    }
//    else
//    {
//      /* Print IO configuration */
//      if (io_oe_cfg | io_term_cfg | io_spec_cfg)
//      {
//        std::cout << "Current " << ioName << " configuration:" << std::endl;
//        if (io_oe_cfg)
//        {
//          io_oe_status = ioctrl->GetOe(ioName); 
//          std::cout << "  Output enable (OE):       ";
//          if (io_oe_status)   { std::cout << "On"  << std::endl; }
//          else                { std::cout << "Off" << std::endl; }
//        }
//        if (io_term_cfg)
//        {
//          io_term_status = ioctrl->GetTerm(ioName); 
//          std::cout << "  Termination (Term):       ";
//          if (io_term_status) { std::cout << "On"  << std::endl; }
//          else                { std::cout << "Off" << std::endl; }
//        }
//        if (io_spec_cfg)
//        {
//          if ((io_direction == IO_CFG_FIELD_DIR_INPUT) || (io_direction == IO_CFG_FIELD_DIR_INOUT))
//          {
//            io_spec_status = ioctrl->GetSpecIn(ioName); 
//            std::cout << "  Special mode in (Spec):   ";
//            if (io_spec_status) { std::cout << "On"  << std::endl; }
//            else                { std::cout << "Off" << std::endl; }
//          }
//          if ((io_direction == IO_CFG_FIELD_DIR_OUTPUT) || (io_direction == IO_CFG_FIELD_DIR_INOUT))
//          {
//            io_spec_status = ioctrl->GetSpecOut(ioName); 
//            std::cout << "  Special mode out (Spec):  ";
//            if (io_spec_status) { std::cout << "On"  << std::endl; }
//            else                { std::cout << "Off" << std::endl; }
//          }
//        }
//      }
//      if ((io_direction == IO_CFG_FIELD_DIR_OUTPUT) || (io_direction == IO_CFG_FIELD_DIR_INOUT))
//      {
//        std::cout << "  Multiplexer (Mux):        ";
//        io_mux_status = ioctrl->GetMux(ioName); 
//        if (io_mux_status)  { std::cout << "BuTiS t0 + TS"  << std::endl; }
//        else                { std::cout << "ECA/IOC/ClkGen" << std::endl; }
//      }
//      else
//      {
//        std::cout << "Note: This IO does not provide any configuration parameters!" << std::endl;
//      }
//      
//      /* Check current state of the IO */
//      if (io_channel != IO_CFG_CHANNEL_FIXED)
//      {
//        std::cout << "Current " << ioName << " status:" << std::endl;
//        /* Print Input/Output */
//        if ((io_direction == IO_CFG_FIELD_DIR_OUTPUT) || (io_direction == IO_CFG_FIELD_DIR_INOUT))
//        {
//          /* Output (combined) */
//          io_current_state = ioctrl->GetOutputCombined(ioName);
//          std::cout << "  Output (combined):        ";
//          if      (io_current_state == 0xff) { std::cout << "High"; }
//          else if (io_current_state == 0x00) { std::cout << "Low "; }
//          else                               { std::cout << "Edge"; }
//          std::cout << " [" << io_current_state << "/0x" << hex << io_current_state << "]" << dec << std::endl;
//          
//          /* Output (IO module) */
//          io_current_state = ioctrl->GetOutput(ioName);
//          std::cout << "  Output (IO module):       ";
//          if      (io_current_state == 0xff) { std::cout << "High"; }
//          else if (io_current_state == 0x00) { std::cout << "Low "; }
//          else                               { std::cout << "Edge"; }
//          std::cout << " [" << io_current_state << "/0x" << hex << io_current_state << "]" << dec << std::endl;
//        }
//        if ((io_direction == IO_CFG_FIELD_DIR_INPUT) || (io_direction == IO_CFG_FIELD_DIR_INOUT))
//        {
//          /* Input  */
//          io_current_state = ioctrl->GetInput(ioName);
//          std::cout << "  Input:                    ";
//          if      (io_current_state == 0xff) { std::cout << "High"; }
//          else if (io_current_state == 0x00) { std::cout << "Low "; }
//          else                               { std::cout << "Edge"; }
//          std::cout << " [" << io_current_state << "/0x" << hex << io_current_state << "]" << dec << std::endl;
//        }
//      }
//    } /* try */
//  }
//  catch (const Glib::Error& error) 
//  {
//    /* Catch error(s) */
//    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
//    return (__IO_RETURN_FAILURE);
//  }
//  
//  /* Done */
//  std::cout << std::endl;
//  return (return_value);
//}
//
///* Function io_print_table() */
///* ==================================================================================================== */
//static int io_print_table(bool verbose_mode)
//{
//  /* Helpers */
//  int ios_total = 0;     /* Total IOs */
//  int item_iterator = 0; /* Iterate through IO table */
//  
//  /* Print help */
//  std::cout << "Listing IOs and capabilities..." << std::endl;
//  std::cout << std::endl;
//  std::cout << "Direction/OE/Term/Special:" << std::endl;
//  std::cout << "  Yes: This value can be modified" << std::endl;
//  std::cout << "  No:  This property is fixed" << std::endl;
//  std::cout << std::endl;
//  std::cout << "Channel:" << std::endl;
//  std::cout << "  GPIO/LVDS: Tells you the ECA/Clock generator channel" << std::endl;
//  std::cout << "  FIXED:     IO behavior is fixed and can't be configured or driven manually" << std::endl;
//  
//  /* Print table header */
//  std::cout << std::endl;
//  if (verbose_mode)
//  {
//    std::cout << "Name          Delay[ns]   Internal ID   Direction   Channel   OE    Term   Special   Logic Level" << std::endl;
//    std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
//  }
//  else
//  {
//    std::cout << "Name          Direction   Channel   OE    Term   Special   Logic Level" << std::endl;
//    std::cout << "----------------------------------------------------------------------" << std::endl;
//  }
//  
//  /* Initialize SAFTLib components */
//  Gio::init();
//  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
//  
//  /* Try to get the table */
//  try
//  {
//    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
//    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
//    map<Glib::ustring, Glib::ustring> ios_proxy = receiver->getInterfaces()["IOControl"];
//    if(ios_proxy.size() != 1)
//    {
//      std::cout << "Interface(s) found: " << ios_proxy.size() << std::endl;
//      std::cout << "Expecting 1 interface only!" << std::endl;
//      return (__IO_RETURN_FAILURE);
//    }
//    Glib::RefPtr<IOControl_Proxy> ioctrl = IOControl_Proxy::create(ios_proxy.begin()->second);
//    ios_total = ioctrl->getGPIOTotal() + ioctrl->getLVDSTotal() + ioctrl->getFIXEDTotal();
//    
//    /* Print every IO with properties */
//    for (item_iterator = 0; item_iterator < ios_total; item_iterator++)
//    {
//      /* Print formated values (setw(MAX_SIZE+ALIGNMENT_CHARS)) */
//      std::cout << std::left;
//      std::cout << std::setw(12+2) << ioctrl->GetIoTableName(item_iterator);
//      if (verbose_mode)
//      {
//        std::cout << std::setw(3+9) << ioctrl->GetIoTableParameterByNumber(item_iterator, eDelay);
//        std::cout << std::setw(3+11) << ioctrl->GetIoTableParameterByNumber(item_iterator, eInternalId);
//        std::cout << std::setw(7+5);
//      }
//      else
//      {
//        std::cout << std::setw(12);
//      }
//      switch(ioctrl->GetIoTableParameterByNumber(item_iterator, eDirection))
//      {
//        case IO_CFG_FIELD_DIR_OUTPUT: { std::cout << "Output  " << std::left; break; }
//        case IO_CFG_FIELD_DIR_INPUT:  { std::cout << "Input   " << std::left; break; }
//        case IO_CFG_FIELD_DIR_INOUT:  { std::cout << "Inout   " << std::left; break; }
//        default:                      { std::cout << "Unknown " << std::left; break; }
//      }
//      std::cout << std::setw(5+5);
//      switch(ioctrl->GetIoTableParameterByNumber(item_iterator, eChannel))
//      {
//        case IO_CFG_CHANNEL_GPIO:  { std::cout << "GPIO  " << std::left; break; }
//        case IO_CFG_CHANNEL_LVDS:  { std::cout << "LVDS  " << std::left; break; }
//        case IO_CFG_CHANNEL_FIXED: { std::cout << "FIXED " << std::left; break; }
//        default:                   { std::cout << "?     " << std::left; break; }
//      }
//      std::cout << std::setw(3+3);
//      switch(ioctrl->GetIoTableParameterByNumber(item_iterator, eOutputEnable))
//      {
//        case IO_CFG_OE_UNAVAILABLE: { std::cout << "No  " << std::left; break; }
//        case IO_CFG_OE_AVAILABLE:   { std::cout << "Yes " << std::left; break; }
//        default:                    { std::cout << "?   " << std::left; break; }
//      }
//      std::cout << std::setw(3+4);
//      switch(ioctrl->GetIoTableParameterByNumber(item_iterator, eTermination))
//      {
//        case IO_CFG_TERM_UNAVAILABLE: { std::cout << "No  " << std::left; break; }
//        case IO_CFG_TERM_AVAILABLE:   { std::cout << "Yes " << std::left; break; }
//        default:                      { std::cout << "?   " << std::left; break; }
//      }
//      std::cout << std::setw(3+7);
//      switch(ioctrl->GetIoTableParameterByNumber(item_iterator, eSpecial))
//      {
//        case IO_CFG_SPEC_UNAVAILABLE: { std::cout << "No  " << std::left; break; }
//        case IO_CFG_SPEC_AVAILABLE:   { std::cout << "Yes " << std::left; break; }
//        default:                      { std::cout << "?   " << std::left; break; }
//      }
//      switch(ioctrl->GetIoTableParameterByNumber(item_iterator, eLogicLevel))
//      {
//        case IO_LOGIC_LEVEL_TTL:   { std::cout << "TTL   " << std::left; break; }
//        case IO_LOGIC_LEVEL_LVTTL: { std::cout << "LVTTL " << std::left; break; }
//        case IO_LOGIC_LEVEL_LVDS:  { std::cout << "LVDS  " << std::left; break; }
//        case IO_LOGIC_LEVEL_NIM:   { std::cout << "NIM   " << std::left; break; }
//        default:                   { std::cout << "?     " << std::left; break; }
//      }
//      std::cout << std::endl;
//    }
//  } 
//  catch (const Glib::Error& error) 
//  {
//    /* Catch error(s) */
//    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
//    return (__IO_RETURN_FAILURE);
//  }
//  
//  /* Done */
//  std::cout << std::endl;
//  return (__IO_RETURN_SUCCESS);
//}

/* Function io_setup() */
/* ==================================================================================================== */
static int  io_setup (int io_oe, int io_term, int io_spec_out, int io_spec_in, int io_mux, int io_drive, 
                      bool set_oe, bool set_term, bool set_spec_out, bool set_spec_in, bool set_mux, bool set_drive,
                      bool verbose_mode)
{
  unsigned io_type = 0; /* Out, Inout or In? */
  bool     io_found = false;
  bool     io_set = false; /* IO set or get */
  
  /* Check if there is at least one parameter to set */
  io_set = set_oe | set_term | io_spec_out | io_spec_in | set_mux | set_drive;
  
  /* Display information */
  if (verbose_mode)
  {
    if (io_set) { std::cout << "Checking configuration feasibility..." << std::endl; }
    else        { std::cout << "Checking current configuration..." << std::endl; }
  }
  
  /* Initialize saftlib components */
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  /* Perform selected action(s) */
  try
  {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    
    /* Search for IO name */
    std::map< Glib::ustring, Glib::ustring > inouts;
    std::map< Glib::ustring, Glib::ustring > outs;
    std::map< Glib::ustring, Glib::ustring > ins;
    Glib::ustring io_path;
    inouts = receiver->getInoutputs();
    outs = receiver->getOutputs();
    ins = receiver->getInputs();
    Glib::RefPtr<Output_Proxy> output_proxy;
    Glib::RefPtr<Input_Proxy> input_proxy;
    
    /* Check if IO exists */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (it->first == ioName) 
      { 
        io_found = true; 
        io_type = IO_CFG_FIELD_DIR_OUTPUT; 
        output_proxy = Output_Proxy::create(it->second);
        io_path = it->second;
      }
    }
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=inouts.begin(); it!=inouts.end(); ++it)
    {
      if (it->first == ioName)
      { 
        io_found = true; 
        io_type = IO_CFG_FIELD_DIR_INOUT;
        output_proxy = Output_Proxy::create(it->second);
        input_proxy = Input_Proxy::create(it->second);
        io_path = it->second;
      }
    }
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=ins.begin(); it!=ins.end(); ++it)
    {
      if (it->first == ioName) 
      { 
        io_found = true; 
        io_type = IO_CFG_FIELD_DIR_INPUT;
        input_proxy = Input_Proxy::create(it->second);
        io_path = it->second;
      }
    }
    
    /* Found IO? */
    if (io_found == false)
    { 
      std::cout << "Error: There is no IO with the name " << ioName << "!" << std::endl; 
      return (__IO_RETURN_FAILURE);
    }
    else
    {
      if (verbose_mode) { std::cout << "Info: Found " << ioName << " (" << io_type << ")" << "!" << std::endl; }
    }
    
    /* Switch: Set something or get the current status) ?*/
    if (io_set)
    {
      /* Check oe configuration */
      if (set_oe)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for outputs!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
        /* Check if OE option is available */
        if (!(output_proxy->getOutputEnableAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
      }
      
      /* Check term configuration */
      if (set_term)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
        {
          std::cout << "Error: This option is not available for outputs!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
        /* Check if TERM option is available */
        if (!(input_proxy->getInputTerminationAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
      }
    
      /* Set configuration */
      if (set_oe)    { output_proxy->setOutputEnable(io_oe); }
      if (set_term)  { input_proxy->setInputTermination(io_term); }
      if (set_drive) { output_proxy->WriteOutput(io_drive); }
      
    }
    else
    {
      /* Generic information */
      std::cout << "IO:" << std::endl;
      std::cout << "  Path: " << io_path << std::endl;
      std::cout << std::endl;
    
      /* Display configuration */
      std::cout << "Current state:" << std::endl; 
      if (io_type != IO_CFG_FIELD_DIR_INPUT)
      {
        /* Display output */
        if (output_proxy->ReadOutput())           { std::cout << "  Output:           High" << std::endl; }
        else                                      { std::cout << "  Output:           Low" << std::endl; }
        
        /* Display output enable state */
        if (output_proxy->getOutputEnableAvailable())
        {
          if (output_proxy->getOutputEnable())    { std::cout << "  OutputEnable:     On" << std::endl; }
          else                                    { std::cout << "  OutputEnable:     Off" << std::endl; }
        }
      }
      
      if (io_type != IO_CFG_FIELD_DIR_OUTPUT)
      {
        /* Display input */
        if (input_proxy->ReadInput())             { std::cout << "  Input:            High" << std::endl; }
        else                                      { std::cout << "  Input:            Low" << std::endl; }
        
        /* Display output enable state */
        if (input_proxy->getInputTerminationAvailable())
        {
          if (input_proxy->getInputTermination()) { std::cout << "  InputTermination: On" << std::endl; }
          else                                    { std::cout << "  InputTermination: Off" << std::endl; }
        }
      }
      
    }
    
  }
  catch (const Glib::Error& error) 
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }
  
  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function io_print_table() */
/* ==================================================================================================== */
static int io_print_table(bool verbose_mode)
{
  /* Initialize saftlib components */
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  /* Try to get the table */
  try
  {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< Glib::ustring, Glib::ustring > inouts;
    std::map< Glib::ustring, Glib::ustring > outs;
    std::map< Glib::ustring, Glib::ustring > ins;
    inouts = receiver->getInoutputs();
    outs = receiver->getOutputs();
    ins = receiver->getInputs();
    
    /* Show verbose output */
    if (verbose_mode)
    {
      std::cout << "Discovered Inoutput(s): " << std::endl;
      for (std::map<Glib::ustring,Glib::ustring>::iterator it=inouts.begin(); it!=inouts.end(); ++it)
      {
        std::cout << it->first << " => " << it->second << '\n';
      }
      std::cout << "Discovered Output(s): " << std::endl;
      for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
      {
        std::cout << it->first << " => " << it->second << '\n';
      }
      std::cout << "Discovered Inputs(s) " << std::endl;
      for (std::map<Glib::ustring,Glib::ustring>::iterator it=ins.begin(); it!=ins.end(); ++it)
      {
        std::cout << it->first << " => " << it->second << '\n';
      }
    }
    
    /* Print help */
    if (verbose_mode) { std::cout << "Listing IOs and capabilities..." << std::endl << std::endl; }
    std::cout << "Device:" << std::endl;
    std::cout << "  Name: " << receiver->getName() << std::endl;
    std::cout << "  Path: " << receiver->getEtherbonePath() << std::endl;
    std::cout << std::endl;
    std::cout << "OutputEnable/InputTermination/SpecialOut/In:" << std::endl;
    std::cout << "  Yes: This value can be modified" << std::endl;
    std::cout << "  No:  This property is fixed" << std::endl;
    std::cout << std::endl;
    std::cout << "Channel:" << std::endl;
    std::cout << "  GPIO/LVDS: Tells you the ECA/Clock generator channel" << std::endl;
    std::cout << "  FIXED:     IO behavior is fixed and can't be configured or driven manually" << std::endl;
    
    /* Print table header */
    std::cout << std::endl;
    std::cout << "Name           Direction  OutputEnable  InputTermination  SpecialOut  SpecialIn  Logic Level" << std::endl;
    std::cout << "--------------------------------------------------------------------------------------------" << std::endl;
    
    /* Print Outputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      Glib::RefPtr<Output_Proxy> output_proxy;
      output_proxy = Output_Proxy::create(it->second);
      std::cout << std::left;
      std::cout << std::setw(12+2) << it->first << " ";
      std::cout << std::setw(5+6)  << "Out ";
      std::cout << std::setw(3+11);
      if(output_proxy->getOutputEnableAvailable()) { std::cout << "Yes"; }
      else                                         { std::cout << "No"; }
      std::cout << std::setw(3+15);
      std::cout << "No"; /* InputTermination */
      std::cout << std::setw(5+7);
      if(output_proxy->getSpecialPurposeOutAvailable()) { std::cout << "Yes"; }
      else                                              { std::cout << "No"; }
      std::cout << std::setw(3+8);
      std::cout << "No"; /* SpecialOut */
      std::cout << output_proxy->getLogicLevelOut();
      std::cout << std::endl;
    }
    
    /* Print Inoutputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=inouts.begin(); it!=inouts.end(); ++it)
    {
      Glib::RefPtr<Output_Proxy> output_proxy;
      output_proxy = Output_Proxy::create(it->second);
      Glib::RefPtr<Input_Proxy> input_proxy;
      input_proxy = Input_Proxy::create(it->second);
      std::cout << std::left;
      std::cout << std::setw(12+2) << it->first << " ";
      std::cout << std::setw(5+6)  << "Inout ";
      std::cout << std::setw(3+11);
      if(output_proxy->getOutputEnableAvailable()) { std::cout << "Yes"; }
      else                                         { std::cout << "No"; }
      std::cout << std::setw(3+15);
      if(input_proxy->getInputTerminationAvailable()) { std::cout << "Yes"; }
      else                                            { std::cout << "No"; }
      std::cout << std::setw(5+7);
      if(output_proxy->getSpecialPurposeOutAvailable()) { std::cout << "Yes"; }
      else                                              { std::cout << "No"; }
      std::cout << std::setw(3+8);
      if(input_proxy->getSpecialPurposeInAvailable()) { std::cout << "Yes"; }
      else                                            { std::cout << "No"; }
      std::cout << output_proxy->getLogicLevelOut();
      std::cout << std::endl;
    }
    
    /* Print Inputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=ins.begin(); it!=ins.end(); ++it)
    {
      Glib::RefPtr<Input_Proxy> input_proxy;
      input_proxy = Input_Proxy::create(it->second);
      std::cout << std::left;
      std::cout << std::setw(12+2) << it->first << " ";
      std::cout << std::setw(5+6)  << "In ";
      std::cout << std::setw(3+11);
      std::cout << "No";
      std::cout << std::setw(3+15);
      if(input_proxy->getInputTerminationAvailable()) { std::cout << "Yes"; }
      else                                            { std::cout << "No"; }
      std::cout << std::setw(5+7);
      std::cout << "No";
      std::cout << std::setw(3+8);
      if(input_proxy->getSpecialPurposeInAvailable()) { std::cout << "Yes"; }
      else                                            { std::cout << "No"; }
      std::cout << input_proxy->getLogicLevelIn();
      std::cout << std::endl;
    }
    
  }
  catch (const Glib::Error& error) 
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }
  
  /* Done */
  return (__IO_RETURN_SUCCESS);
}

/* Function main() */
/* ==================================================================================================== */
int main (int argc, char** argv)
{
  /* Helpers */
  int  opt          = 0;     /* Number of given options */
  int  io_oe        = 0;     /* Output enable */
  int  io_term      = 0;     /* Input Termination */
  int  io_spec_out  = 0;     /* Special (output) function */
  int  io_spec_in   = 0;     /* Special (input) function */
  int  io_mux       = 0;     /* Multiplexer (BuTiS) */
  int  io_drive     = 0;     /* Drive IO value */
  int  return_code  = 0;     /* Return code */
  bool set_oe       = false; /* Set? */
  bool set_term     = false; /* Set? */
  bool set_spec_in  = false; /* Set? */
  bool set_spec_out = false; /* Set? */
  bool set_mux      = false; /* Set? */
  bool set_drive    = false; /* Set? */
  bool verbose_mode = false; /* Print verbose output to output stream => -v */
  bool show_help    = false; /* Print help => -h */
  bool show_table   = false; /* Print io mapping table => -i */
  
  /* Get the application name */
  program = argv[0]; 
  
  /* Get basic arguments, we need at least the device name */
  if (argc >= 2)
  {
    deviceName = argv[1]; /* Get the device name */
    if (argc >= 3)
    { 
      /* Get the io name */
      ioName = argv[2];
      /* Prevent wrong argument usage */
      if (!strcmp("-v",ioName)) { show_help  = true; }
      if (!strcmp("-h",ioName)) { show_help  = true; }
      if (!strcmp("-i",ioName)) { show_table = true; }
    }
    else
    {
      show_help = true; 
      std::cout << "Missing arguments (at least one option or IO name)..." << std::endl;
    }
  }
  else
  {
    show_help = true;
    std::cout << "Missing arguments (at least a device name)..." << std::endl;
  }
  
  /* Parse for options */
  while ((opt = getopt(argc, argv, "o:t:p:e:m:d:vhi")) != -1)
  {
    switch (opt)
    {
      case 'o': { io_oe        = atoi(optarg); set_oe       = true; break; }
      case 't': { io_term      = atoi(optarg); set_term     = true; break; }
      case 'p': { io_spec_out  = atoi(optarg); set_spec_out = true; break; }
      case 'e': { io_spec_in   = atoi(optarg); set_spec_in  = true; break; }
      case 'm': { io_mux       = atoi(optarg); set_mux      = true; break; }
      case 'd': { io_drive     = atoi(optarg); set_drive    = true; break; }
      case 'v': { verbose_mode = true; break; }
      case 'h': { show_help    = true; break; }
      case 'i': { show_table   = true; break; }
      default:  { std::cout << "Unknown argument..." << std::endl; show_help = true; break; }
    }
    /* Break loop if help is needed */
    if (show_help) { break; }
  }
  
  /* Check if help is needed, otherwise evaluate given arguments */
  if (show_help) { io_help(); }
  else
  {
    /* Plausibility check */
    if (io_oe > 1       || io_oe < 0)       { std::cout << "Error: Output enable setting is invalid!"             << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_term > 1     || io_term < 0)     { std::cout << "Error: Input termination setting is invalid"          << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_spec_in > 1  || io_spec_in < 0)  { std::cout << "Error: Special (input) function setting is invalid!"  << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_spec_out > 1 || io_spec_out < 0) { std::cout << "Error: Special (output) function setting is invalid!" << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_mux > 1      || io_mux < 0)      { std::cout << "Error: BuTiS t0 multiplexer setting is invalid!"      << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_drive > 1    || io_drive < 0)    { std::cout << "Error: Output value is not valid!"                    << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    
    /* Proceed with program */
    if (show_help) { io_help(); }
    else
    { 
      if(show_table) { return_code = io_print_table(verbose_mode); }
      else           { return_code = io_setup(io_oe, io_term, io_spec_out, io_spec_in, io_mux, io_drive, set_oe, set_term, set_spec_out, set_spec_in, set_mux, set_drive, verbose_mode); }
    }
  }
  
  /* Done */
  return (return_code);
}
