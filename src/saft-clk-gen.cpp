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
#include "interfaces/OutputCondition.h"
#include "interfaces/Input.h"

/* Namespace */
/* ==================================================================================================== */
using namespace saftlib;
using namespace std;

/* Global */
/* ==================================================================================================== */
static const char *ioName      = NULL;  /* Name of the IO */
static bool        ioNameGiven = false; /* IO name given? */
static const char *program     = NULL;  /* Name of the application */
static const char *deviceName  = NULL;  /* Name of the device */

/* Function clk_show_table() */
/* ==================================================================================================== */
static int clk_show_table (void)
{
  /* Initialize saftlib components */
  
  /* Try to get the table */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< std::string, std::string > outs;
    outs = receiver->getOutputs();
    
    /* Print table header */
    std::cout << "Name           Logic Level" << std::endl;
    std::cout << "--------------------------" << std::endl;
    
    /* Print Outputs */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        std::shared_ptr<Output_Proxy> output_proxy;
        output_proxy = Output_Proxy::create(it->second);
        if (output_proxy->getTypeOut() == "1ns (LVDS)")
        {
          std::cout << std::left;
          std::cout << std::setw(12+2) << it->first << " ";
          std::cout << output_proxy->getLogicLevelOut();
          std::cout << std::endl;
        }
      }
    }
    
  }
  catch (const Glib::Error& error) 
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (-1);
  }
  
  /* Done */
  return (0);
}

/* Function clk_configure() */
/* ==================================================================================================== */
static int clk_configure(double high_phase, double low_phase, uint64_t phase_offset)
{
  /* Helper */
  bool ioFound = false;
  bool ioClkStatus = false;
  
  /* Check if IO name was given */
  if (!ioNameGiven) 
  {
    std::cerr << "Missing IO name!" << std::endl;
    return (-1);
  }

  /* Initialize saftlib components */
  
  /* Try to get the table */
  try
  {
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< std::string, std::string > outs;
    outs = receiver->getOutputs();
    
    /* Configure clock */
    for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if ((ioNameGiven && (it->first == ioName)))
      {
        ioFound = true;
        std::shared_ptr<Output_Proxy> output_proxy;
        output_proxy = Output_Proxy::create(it->second);
        if   ((high_phase == 0.0) && (low_phase == 0.0) && (phase_offset == 0)) { ioClkStatus = output_proxy->StopClock(); }
        else                                                                    { ioClkStatus = output_proxy->StartClock(high_phase, low_phase, phase_offset); }
        if   (ioClkStatus) { std::cout << "Clock started..." << std::endl; }
        else               { std::cout << "Clock stopped..." << std::endl; }
      } 
    }
  }
  catch (const Glib::Error& error) 
  {
    /* Catch error(s) */
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
    return (-1);
  }

  /* Done */
  if (!ioFound) { std::cerr << "IO " << ioName << " does not exist!" << std::endl; return (-1); }
  else          { return (0); }
}

/* Function clk_gen_help() */
/* ==================================================================================================== */
static void clk_gen_help (void)
{
  /* Print arguments and options */
  std::cout << "CLK-GEN for SAFTlib" << std::endl;
  std::cout << "Usage: " << program << " <unique device name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -n <name>:                                              Specify IO name " << std::endl;
  std::cout << "  -p <high phase[ns]> <low phase[ns]> <phase offset[ns]>: Start/Configure clock with the given phases" << std::endl;
  std::cout << "  -f <frequency[Hz]> <phase offset[ns]>                   Start/Configure clock with the given frequency" << std::endl;
  std::cout << "  -s:                                                     Stop clock for the given IO" << std::endl;
  std::cout << "  -i:                                                     List all clock generator outputs" << std::endl;
  std::cout << "  -v:                                                     Switch to verbose mode" << std::endl;
  std::cout << "  -h:                                                     Print help (this message)" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl;
  std::cout << program << " exploder5a_123t " << "-n IO1 " << "-p 4 4 2" << std::endl;
  std::cout << "  This will generate a 125MHz clock (with a 2ns phase offset)" << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <csco-tg@gsi.de>" << std::endl;
  std::cout << "Licensed under the GPLv3" << std::endl;
  std::cout << std::endl;
}

/* Function main() */
/* ==================================================================================================== */
int main (int argc, char** argv)
{
  /* Helpers */
  char     *pEnd            = NULL;
  int      opt              = 0;
  bool     start_clock      = false;
  bool     stop_clock       = false;
  bool     show_help        = false;
  bool     phase_config     = false;
  bool     frequency_config = false;
  bool     show_table       = false;
  bool     verbose_mode     = false;
  double   high_phase       = 0.0;
  double   low_phase        = 0.0;
  uint64_t phase_offset     = 0;
  double   frequency        = 0;
  int      return_code      = 0;

  /* Get the application name */
  program = argv[0]; 
  deviceName = "NoDeviceNameGiven";
  ioName = "NoIONameGiven";
  
  /* Parse for options */
  while ((opt = getopt(argc, argv, "n:p:f:sivh")) != -1)
  {
    switch (opt)
    {
      case 'n': { ioName           = argv[optind-1]; ioNameGiven  = true; break; }
      case 'p': { start_clock      = true;
                  phase_config     = true;
                  if (argv[optind-1] != NULL) { high_phase = strtod(argv[optind-1], &pEnd); }
                  else                        { std::cerr << "Error: Missing value for high phase[ns]!" << std::endl; return (-1); }
                  if (argv[optind-0] != NULL) { low_phase = strtod(argv[optind-0], &pEnd); }
                  else                        { std::cerr << "Error: Missing value for low phase[ns]!" << std::endl; return (-1); }
                  if (argv[optind+1] != NULL) { phase_offset = strtoull(argv[optind+1], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing value for phase offset[ns]!" << std::endl; return (-1); }
                  break; }
      case 'f': { start_clock      = true;
                  frequency_config = true;
                  if (argv[optind-1] != NULL) { frequency = strtod(argv[optind-1], &pEnd); }
                  else                        { std::cerr << "Error: Missing value for frequency[Hz]!" << std::endl; return (-1); }
                  if (argv[optind-0] != NULL) { phase_offset = strtoull(argv[optind+0], &pEnd, 0); }
                  else                        { std::cerr << "Error: Missing value for phase offset[ns]!" << std::endl; return (-1); }
                  break; }
      case 's': { stop_clock       = true; break; }
      case 'i': { show_table       = true; break; }
      case 'v': { verbose_mode     = true; break; }
      case 'h': { show_help        = true; break; }
      default:  { std::cout << "Unknown argument..." << std::endl; break; }
    }
  }
  
  /* Help wanted? */
  if (show_help) { clk_gen_help(); return (0); }
  
  /* Get basic arguments, we need at least the device name */
  deviceName = argv[optind];
  
  /* Plausibility check for arguments */
  if (start_clock && stop_clock)        { std::cerr << "Error: Start or stop the clock generator" << std::endl; return (-1); }
  if (phase_config && frequency_config) { std::cerr << "Error: Choose phase(s) or frequency!" << std::endl; return (-1); }
  
  /* Calculate phases if frequency is given */
  if (frequency_config)
  {
    high_phase = ( ((1.0/(double)frequency)) * 1000000000.0)/2.0;
    low_phase = high_phase; /* 50/50 duty cycle */
  }
  
  /* Check if device name exists */
  if (deviceName == NULL)
  { 
    std::cerr << "Missing device name!" << std::endl;
    return (-1);
  }
  Gio::init();
  map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
  if (devices.find(deviceName) == devices.end())
  {
    std::cerr << "Device " << deviceName << " does not exist!" << std::endl;
    return (-1);
  }
  
  /* Print additional information */
  if (verbose_mode)
  {
    std::cout << "Settings:" << std::endl;
    std::cout << "  start_clock      = " << start_clock << std::endl;
    std::cout << "  stop_clock       = " << stop_clock << std::endl;
    std::cout << "  phase_config     = " << phase_config << std::endl;
    std::cout << "  frequency_config = " << frequency_config << std::endl;
    std::cout << "  frequency        = " << frequency << std::endl;
    std::cout << "  high_phase       = " << high_phase << std::endl;
    std::cout << "  low_phase        = " << low_phase << std::endl;
    std::cout << "  phase_offset     = " << phase_offset << std::endl;
    std::cout << std::endl;
  }
  
  /* Proceed with program */
  if      (show_help)     { clk_gen_help(); }
  else if (show_table)    { return_code = clk_show_table(); }
  else if (start_clock)   { return_code = clk_configure(high_phase, low_phase, phase_offset); }
  else if (stop_clock)    { return_code = clk_configure(0.0,        0.0,       0); }
  else                    { clk_gen_help(); }
  
  /* Done */
  return (return_code);
}
