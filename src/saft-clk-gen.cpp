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
static const char   *ioName       = NULL;  /* Name of the IO */
static bool          ioNameGiven  = false; /* IO name given? */

/* Global */
/* ==================================================================================================== */
static const char *program    = NULL;  /* Name of the application */
static const char *deviceName = NULL;  /* Name of the device */


static int clk_show_table (void)
{
  /* Initialize saftlib components */
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  /* Try to get the table */
  try
  {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    std::map< Glib::ustring, Glib::ustring > outs;
    outs = receiver->getOutputs();
    
    /* Print table header */
    std::cout << "Name           Logic Level" << std::endl;
    std::cout << "--------------------------" << std::endl;
    
    /* Print Outputs */
    for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
    {
      if (((ioNameGiven && (it->first == ioName)) || !ioNameGiven))
      {
        Glib::RefPtr<Output_Proxy> output_proxy;
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

static int clk_configure(double high_phase, double low_phase, uint64_t phase_offset)
{









  // !!! check if running, return state or error?
  
  return 0;
}


/* Function pps_help() */
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
  std::cout << "  -h:                                                     Print help (this message)" << std::endl;
  std::cout << std::endl;
  std::cout << "Example: " << program << " exploder5a_123t " << std::endl;
  std::cout << "  !!!!!!!!!!!!!" << std::endl;
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
  double   high_phase       = 0.0;
  double   low_phase        = 0.0;
  uint64_t phase_offset     = 0;
  uint64_t frequency        = 0;
  int      return_code      = 0;

  /* Get the application name */
  program = argv[0]; 
  deviceName = "NoDeviceNameGiven";
  ioName = "NoIONameGiven";
  
  /* Parse for options */
  while ((opt = getopt(argc, argv, "n:p:f:sih")) != -1)
  {
    switch (opt)
    {
      case 'n': { ioName           = argv[optind-1]; ioNameGiven  = true; break; }
      case 'p': { start_clock      = true;
                  phase_config     = true;
                  high_phase       = strtod(argv[optind-1], &pEnd);
                  low_phase        = strtod(argv[optind-0], &pEnd);
                  phase_offset     = strtoull(argv[optind+1], &pEnd, 0);
                  break; }
      case 'f': { start_clock      = true;
                  frequency_config = true;
                  frequency        = strtoull(argv[optind-1], &pEnd, 0);
                  phase_offset     = strtoull(argv[optind+0], &pEnd, 0);
                  break; }
      case 's': { stop_clock       = true; break; }
      case 'i': { show_table       = true; break; }
      case 'h': { show_help        = true; break; }
      default:  { std::cout << "Unknown argument..." << std::endl; break; }
    }
    /* Break loop if help is needed */
    if (show_help) { break; }
  }
  
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
  Gio::init();
  map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
  if (devices.find(deviceName) == devices.end())
  {
    std::cerr << "Device " << deviceName << " does not exist!" << std::endl;
    return (-1);
  }
  
#if 1
  std::cout << "deviceName       = " << deviceName << std::endl;
  std::cout << "ioName           = " << ioName << std::endl;
  std::cout << "start_clock      = " << start_clock << std::endl;
  std::cout << "stop_clock       = " << stop_clock << std::endl;
  std::cout << "phase_config     = " << phase_config << std::endl;
  std::cout << "frequency_config = " << frequency_config << std::endl;
  std::cout << "frequency        = " << frequency << std::endl;
  std::cout << "high_phase       = " << high_phase << std::endl;
  std::cout << "low_phase        = " << low_phase << std::endl;
  std::cout << "phase_offset     = " << phase_offset << std::endl;
  std::cout << "show_table       = " << show_table << std::endl;
  std::cout << "show_help        = " << show_help << std::endl;
#endif
  
  /* Proceed with program */
  if      (show_help)     { clk_gen_help(); }
  else if (show_table)    { return_code = clk_show_table(); }
  else if (start_clock)   { return_code = clk_configure(high_phase, low_phase, phase_offset); }
  else if (stop_clock)    { return_code = clk_configure(0.0,        0.0,       0); }
  else                    { clk_gen_help(); }
  
  /* Done */
  return (return_code);
}
