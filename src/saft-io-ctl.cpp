/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include "interfaces/Output.h"
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
  std::cout << "  -s: Snoop on input" << std::endl;
  std::cout << std::endl;
  std::cout << "Example: " << program << " exploder5a_123t " << "IO1 " << "-o 1 -t 0 -d 1" << std::endl;
  std::cout << "  This will enable the output and disable the input termination and drive the output high" << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <csco-tg@gsi.de>" << std::endl;
  std::cout << "Licensed under the GPLv3" << std::endl;
  std::cout << std::endl;
}

static void catch_input(guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  guint64 time = deadline - 5000;
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  static char full[80];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns);
  
  std::cout << ioName << " went " << ((event&1)?"high":"low ") << " at " << date << full << std::endl;
}

/* Funciton io_snoop() */
static int io_snoop()
{
  try {
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    
    std::map< Glib::ustring, Glib::ustring > inputs = receiver->getInputs();
    if (inputs.find(ioName) == inputs.end()) {
      std::cerr << "IO '" << ioName << "' is not an input" << std::endl;
      return __IO_RETURN_FAILURE;
    }
    
    guint64 prefix = UINT64_C(0xfffe000000000000) + (random() << 1);
    // Create a SoftwareCondition to catch the event
    Glib::RefPtr<SoftwareActionSink_Proxy> sas = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
    Glib::RefPtr<SoftwareCondition_Proxy> cond = SoftwareCondition_Proxy::create(sas->NewCondition(true, prefix, -2, 5000));
    cond->Action.connect(sigc::ptr_fun(&catch_input));
    
    // Setup the event
    Glib::RefPtr<Input_Proxy> input = Input_Proxy::create(inputs[ioName]);
    input->setInputTermination(true);
    input->setEventEnable(false);
    input->setEventPrefix(prefix);
    input->setEventEnable(true);
    
    Glib::ustring output_path = input->getOutput();
    if (output_path != "") {
      Glib::RefPtr<Output_Proxy> output = Output_Proxy::create(output_path);
      output->setOutputEnable(false);
    }
    
    // run the loop printing IO events
    loop->run();
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (__IO_RETURN_FAILURE);
  }
  
  return (__IO_RETURN_SUCCESS);
}

/* Function io_setup() */
/* ==================================================================================================== */
static int  io_setup (int io_oe, int io_term, int io_spec_out, int io_spec_in, int io_mux, int io_drive, 
                      bool set_oe, bool set_term, bool set_spec_out, bool set_spec_in, bool set_mux, bool set_drive,
                      bool verbose_mode)
{
  unsigned io_type  = 0;     /* Out, Inout or In? */
  bool     io_found = false; /* IO exists? */
  bool     io_set   = false; /* IO set or get configuration */
  
  /* Check if there is at least one parameter to set */
  io_set = set_oe | set_term | set_spec_out | set_spec_in | set_mux | set_drive;
  
  /* Display information */
  if (verbose_mode)
  {
    if (io_set) { std::cout << "Checking configuration feasibility..." << std::endl; }
    else        { std::cout << "Checking current configuration..." << std::endl; }
  }
  
  /* Plausibility check for io name */
  if (ioName == NULL)
  {
    std::cout << "Error: No IO name provided!" << std::endl; 
    return (__IO_RETURN_FAILURE);
  }
  
  /* Initialize saftlib components */
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  /* Perform selected action(s) */
  try
  {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    
    /* Search for IO name */
    std::map< Glib::ustring, Glib::ustring > outs;
    std::map< Glib::ustring, Glib::ustring > ins;
    Glib::ustring io_path;
    Glib::ustring io_partner;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();
    Glib::RefPtr<Output_Proxy> output_proxy;
    Glib::RefPtr<Input_Proxy> input_proxy;
    
    /* Check if IO exists as input */
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
    
    /* Check if IO exists output */
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
    
    /* Check if IO is bidirectional */
    if (io_type == IO_CFG_FIELD_DIR_INPUT)
    {
      io_partner = input_proxy->getOutput();
      if (io_partner != "") { io_type = IO_CFG_FIELD_DIR_INOUT; }
    }
    else if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
    {
      io_partner = output_proxy->getInput();
      if (io_partner != "") { io_type = IO_CFG_FIELD_DIR_INOUT; }
    }
    else
    {
      std::cout << "IO direction is unknown!" << std::endl;
      return (__IO_RETURN_FAILURE);
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
          std::cout << "Error: This option is not available for inputs!" << std::endl; 
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
      
      /* Check special out configuration */
      if (set_spec_out)
      {
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
        /* Check if SPEC OUT option is available */
        if (!(output_proxy->getSpecialPurposeOutAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
      }
      
      /* Check special out configuration */
      if (set_spec_in)
      {
        if (io_type == IO_CFG_FIELD_DIR_OUTPUT)
        {
          std::cout << "Error: This option is not available for outputs!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
        /* Check if SPEC OUT option is available */
        if (!(input_proxy->getSpecialPurposeInAvailable()))
        {
          std::cout << "Error: This option does not exist for this IO!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
      }
      
      /* Check multiplexer (BuTiS support) */
      if (set_mux)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
      }
      
      /* Check if IO can be driven */
      if (set_drive)
      {
        /* Plausibility check */
        if (io_type == IO_CFG_FIELD_DIR_INPUT)
        {
          std::cout << "Error: This option is not available for inputs!" << std::endl; 
          return (__IO_RETURN_FAILURE);
        }
      }
      
      /* Set configuration */
      if (set_oe)       { output_proxy->setOutputEnable(io_oe); }
      if (set_term)     { input_proxy->setInputTermination(io_term); }
      if (set_spec_out) { output_proxy->setSpecialPurposeOut(io_spec_out); }
      if (set_spec_in)  { input_proxy->setSpecialPurposeIn(io_spec_in); }
      if (set_mux)      { output_proxy->setBuTiSMultiplexer(io_mux); }
      if (set_drive)    { output_proxy->WriteOutput(io_drive); }
    }
    else
    {
      /* Generic information */
      std::cout << "IO:" << std::endl;
      std::cout << "  Path:    " << io_path << std::endl;
      if (io_partner != "") { std::cout << "  Partner: " << io_partner << std::endl; }
      std::cout << std::endl;
    
      /* Display configuration */
      std::cout << "Current state:" << std::endl; 
      if (io_type != IO_CFG_FIELD_DIR_INPUT)
      {
        /* Display output */
        if (output_proxy->ReadOutput())             { std::cout << "  Output:           High" << std::endl; }
        else                                        { std::cout << "  Output:           Low" << std::endl; }
        
        /* Display output enable state */
        if (output_proxy->getOutputEnableAvailable())
        {
          if (output_proxy->getOutputEnable())      { std::cout << "  OutputEnable:     On" << std::endl; }
          else                                      { std::cout << "  OutputEnable:     Off" << std::endl; }
        }
        
        /* Display special out state */
        if (output_proxy->getSpecialPurposeOutAvailable())
        {
          if (output_proxy->getSpecialPurposeOut()) { std::cout << "  SpecialOut:       On" << std::endl; }
          else                                      { std::cout << "  SpecialOut:       Off" << std::endl; }
        }
        
        /* Display BuTiS multiplexer state */
        if (output_proxy->getBuTiSMultiplexer())    { std::cout << "  Multiplexer:      BuTiS t0 + TS" << std::endl; }
        else                                        { std::cout << "  Multiplexer:      ECA/IOC/ClkGen" << std::endl; }
      }
      
      if (io_type != IO_CFG_FIELD_DIR_OUTPUT)
      {
        /* Display input */
        if (input_proxy->ReadInput())               { std::cout << "  Input:            High" << std::endl; }
        else                                        { std::cout << "  Input:            Low" << std::endl; }
        
        /* Display output enable state */
        if (input_proxy->getInputTerminationAvailable())
        {
          if (input_proxy->getInputTermination())   { std::cout << "  InputTermination: On" << std::endl; }
          else                                      { std::cout << "  InputTermination: Off" << std::endl; }
        }
        
        /* Display special in state */
        if (input_proxy->getSpecialPurposeInAvailable())
        {
          if (input_proxy->getSpecialPurposeIn())   { std::cout << "  SpecialIn:        On" << std::endl; }
          else                                      { std::cout << "  SpecialIn:        Off" << std::endl; }
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
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  /* Try to get the table */
  try
  {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< Glib::ustring, Glib::ustring > outs;
    std::map< Glib::ustring, Glib::ustring > ins;
    outs = receiver->getOutputs();
    ins = receiver->getInputs();
    
    /* Show verbose output */
    if (verbose_mode)
    {
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
    std::cout << "  Path: " << receiver->getEtherbonePath() << std::endl;
    std::cout << std::endl;
    std::cout << "OutputEnable/InputTermination/SpecialOut/In:" << std::endl;
    std::cout << "  Yes: This value can be modified" << std::endl;
    std::cout << "  No:  This property is fixed" << std::endl;
    std::cout << std::endl;
    std::cout << "Channel:" << std::endl;
    std::cout << "  GPIO/LVDS: Tells you the ECA/Clock generator channel (for legacy tools)" << std::endl;
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
  bool snoop        = false; /* Snoop on an input */
  bool verbose_mode = false; /* Print verbose output to output stream => -v */
  bool show_help    = false; /* Print help => -h */
  bool show_table   = false; /* Print io mapping table => -i */
  
  /* Get the application name */
  program = argv[0]; 
  
  /* Parse for options */
  while ((opt = getopt(argc, argv, "o:t:p:e:m:d:svhi")) != -1)
  {
    switch (opt)
    {
      case 'o': { io_oe        = atoi(optarg); set_oe       = true; break; }
      case 't': { io_term      = atoi(optarg); set_term     = true; break; }
      case 'p': { io_spec_out  = atoi(optarg); set_spec_out = true; break; }
      case 'e': { io_spec_in   = atoi(optarg); set_spec_in  = true; break; }
      case 'm': { io_mux       = atoi(optarg); set_mux      = true; break; }
      case 'd': { io_drive     = atoi(optarg); set_drive    = true; break; }
      case 's': { snoop        = true; break; }
      case 'v': { verbose_mode = true; break; }
      case 'h': { show_help    = true; break; }
      case 'i': { show_table   = true; break; }
      default:  { std::cout << "Unknown argument..." << std::endl; show_help = true; break; }
    }
    /* Break loop if help is needed */
    if (show_help) { break; }
  }
  
  /* Get basic arguments, we need at least the device name */
  if (optind + 1 == argc)
  { 
    deviceName = argv[optind]; /* Get the device name */
    if ((io_oe || io_term || io_spec_out || io_spec_in || io_mux || io_drive || show_help || show_table || snoop) == false)
    {
      show_help = true;
      std::cout << "Incorrect non-optional arguments (expecting at least the device name and one additional argument)..." << std::endl;
    }
  }
  else if (optind + 2 == argc)
  {
    deviceName = argv[optind]; /* Get the device name */
    ioName = argv[optind+1]; /* Get the io name */
  }
  else 
  { 
    show_help = true;
    std::cout << "Incorrect non-optional arguments (expecting at least the device name and one additional argument)..." << std::endl;
  }
  
  Gio::init();
  /* Check if help is needed, otherwise evaluate given arguments */
  if (show_help) { io_help(); }
  else
  {
    /* Confirm device exists */
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    if (devices.find(deviceName) == devices.end()) {
      std::cerr << "Device '" << deviceName << "' does not exist" << std::endl;
      return -1;
    }
    
    /* Plausibility check */
    if (io_oe > 1       || io_oe < 0)       { std::cout << "Error: Output enable setting is invalid!"             << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_term > 1     || io_term < 0)     { std::cout << "Error: Input termination setting is invalid"          << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_spec_out > 1 || io_spec_out < 0) { std::cout << "Error: Special (output) function setting is invalid!" << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_spec_in > 1  || io_spec_in < 0)  { std::cout << "Error: Special (input) function setting is invalid!"  << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_mux > 1      || io_mux < 0)      { std::cout << "Error: BuTiS t0 multiplexer setting is invalid!"      << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    if (io_drive > 1    || io_drive < 0)    { std::cout << "Error: Output value is not valid!"                    << std::endl; show_help = true; return_code = __IO_RETURN_FAILURE; }
    
    /* Proceed with program */
    if (show_help) { io_help(); }
    else
    { 
      if(show_table) { return_code = io_print_table(verbose_mode); }
      else if (snoop){ return_code = io_snoop(); }
      else           { return_code = io_setup(io_oe, io_term, io_spec_out, io_spec_in, io_mux, io_drive, set_oe, set_term, set_spec_out, set_spec_in, set_mux, set_drive, verbose_mode); }
    }
  }
  
  /* Done */
  return (return_code);
}
