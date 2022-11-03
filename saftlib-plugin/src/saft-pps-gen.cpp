/* Synopsis */
/* ==================================================================================================== */
/* IO control application */

/* Defines */
/* ==================================================================================================== */
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#define ECA_EVENT_ID	UINT64_C(0x1fff000000000000) /* FID=1 & GRPID=MAX */
#define ECA_EVENT_MASK	UINT64_C(0xffff000000000000)

/* Includes */
/* ==================================================================================================== */
#include <iostream>
#include <iomanip>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "CommonFunctions.hpp"
#include <saftbus/error.hpp>

#include "SAFTd_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SoftwareActionSink_Proxy.hpp"
#include "SoftwareCondition_Proxy.hpp"
#include "Output_Proxy.hpp"
#include "OutputCondition_Proxy.hpp"
#include "Input_Proxy.hpp"
#include "SCUbusActionSink_Proxy.hpp"
#include "SCUbusCondition_Proxy.hpp"

/* Namespace */
/* ==================================================================================================== */
using namespace saftlib;
using namespace std;

/* Global */
/* ==================================================================================================== */
static const char *program    = NULL;  /* Name of the application */
static const char *deviceName = NULL;  /* Name of the device */
bool verbose_mode             = false; /* Print verbose output to output stream => -v */
bool UTC                      = false; /* Print absolute time in UTC instead of TAI */
uint64_t overflow_counter      = 0;
uint64_t action_counter        = 0;
uint64_t late_counter          = 0;
uint64_t early_counter         = 0;
uint64_t conflict_counter      = 0;
uint64_t delayed_counter       = 0;

/* Prototypes */
/* ==================================================================================================== */
void onAction(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags, int rule, std::shared_ptr<SoftwareActionSink_Proxy> sink);
void onOverflowCount(uint64_t count);
void onActionCount(uint64_t count);
void onLateCount(uint64_t count);
void onEarlyCount(uint64_t count);
void onConflictCount(uint64_t count);
void onDelayedCount(uint64_t count);
static void pps_help (void);


/* Function onAction() */
/* ==================================================================================================== */
void onAction(uint64_t event, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags, int rule, std::shared_ptr<SoftwareActionSink_Proxy> sink)
{
  std::cout << "Got event at: 0x" << std::hex << (UTC?executed.getUTC():executed.getTAI()) << " -> " << tr_formatDate(executed, (verbose_mode?PMODE_VERBOSE:PMODE_NONE) | (UTC?PMODE_UTC:PMODE_NONE) ) << std::endl;
  if (verbose_mode)
  {
    std::cout << "  Deadline (Diff.): 0x" << (UTC?deadline.getUTC():deadline.getTAI()) << " (0x" << (deadline-executed) << ")" << std::endl;
    std::cout << "  Flags:            0x" << flags << std::endl;
    std::cout << "  Rule:             0x" << rule << std::endl;
    std::cout << std::dec;
    std::cout << "  Action Counter:   " << sink->getActionCount() << std::endl;
    std::cout << "  Overflow Counter: " << sink->getOverflowCount() << std::endl;
    std::cout << "  Late Counter:     " << sink->getLateCount() << std::endl;
    std::cout << "  Early Counter:    " << sink->getEarlyCount() << std::endl;
    std::cout << "  Conflict Counter: " << sink->getConflictCount() << std::endl;
    std::cout << "  Delayed Counter:  " << sink->getDelayedCount() << std::endl;
  }
}

/* Generic counter functions */
/* ==================================================================================================== */
void onActionCount(uint64_t count)   { action_counter++; }
void onOverflowCount(uint64_t count) { overflow_counter++; }
void onLateCount(uint64_t count)     { late_counter++; }
void onEarlyCount(uint64_t count)    { early_counter++; }
void onConflictCount(uint64_t count) { conflict_counter++; }
void onDelayedCount(uint64_t count)  { delayed_counter++; }

/* Function pps_help() */
/* ==================================================================================================== */
static void pps_help (void)
{
  /* Print arguments and options */
  std::cout << "PPS-GEN for SAFTlib " << std::endl;
  std::cout << "Usage: " << program << " <unique device name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -s        : Turn output enable on and input termination off" << std::endl;
  std::cout << "  -e        : External event mode (ECA configuration only)" << std::endl;
  std::cout << "  -t <tag>  : Set up a new condition on the SCU bus (uint32 tag injection)" << std::endl;
  std::cout << "  -i        : Inject PPS event(s) without creating conditions" << std::endl;
  std::cout << "  -U        : display time in UTC instead of TAI" << std::endl;
  std::cout << "  -h        : Print help (this message)" << std::endl;
  std::cout << "  -v        : Switch to verbose mode" << std::endl;
  std::cout << std::endl;
  std::cout << "Example: " << program << " exploder5a_123t " << std::endl;
  std::cout << "  This will output one pulse per second on every IO." << std::endl;
  std::cout << std::endl;
  std::cout << "Expected Event ID: " << std::endl;
  std::cout << "  0x" << std::hex << ECA_EVENT_ID << std::dec << " (" << ECA_EVENT_ID << ")" << std::endl;
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
  int  opt              = 0;     /* Number of given options */
  int  total_ios        = 0;     /* Number of configured IOs */
  bool setup_io         = false; /* Setup OE and TERM? */
  bool external_trigger = false; /* Self-triggered or trigged by an external event */
  bool just_inject      = false; /* Just inject or also create condition(s) */
  bool show_help        = false; /* Print help => -h */
  bool first_pps        = true;  /* Is this the first PPS output? */
  bool wrLocked         = false; /* Is the timing receiver locked? */
  bool setup_scu_bus    = false; /* Set up a new condition for the SCU bus? */
  uint32_t scu_bus_tag  = 0;     /* SCU Bus tag */
  saftlib::Time wrTime;          /* Current time */
  saftlib::Time wrNext;          /* Execution time for the next PPS */

  /* Get the application name */
  program = argv[0]; 
  
  /* Parse for options */
  while ((opt = getopt(argc, argv, ":seiUvht:")) != -1)
  {
    switch (opt)
    {
      case 's': { setup_io         = true; break; }
      case 'e': { external_trigger = true; break; }
      case 'i': { just_inject      = true; break; }
      case 'U': { UTC              = true; break; }
      case 'v': { verbose_mode     = true; break; }
      case 'h': { show_help        = true; break; }
      case 't': { setup_scu_bus    = true; scu_bus_tag = strtoul(optarg, NULL, 0); break; }
      default:  { std::cout << "Unknown argument..." << std::endl; show_help = true; break; }
    }
    /* Break loop if help is needed */
    if (show_help) { break; }
  }
  
  /* Get basic arguments, we need at least the device name */
  if (optind + 1 == argc)
  { 
    deviceName = argv[optind]; /* Get the device name */
  }
  else 
  { 
    show_help = true;
    std::cout << "Incorrect non-optional arguments..." << std::endl;
  }
  
  /* Check if help is needed, otherwise evaluate given arguments */
  if (show_help)
  { 
    pps_help();
  }
  else
  {
    
    /* Try to setup all outputs */
    try
    {
      map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
      if (devices.find(deviceName) == devices.end())
      {
        std::cerr << "Device '" << deviceName << "' does not exist!" << std::endl;
        return (-1);
      }
      std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
      
      /* Check if timing receiver is locked */
      wrLocked = receiver->getLocked();
      if (wrLocked)
      {
        wrTime  = receiver->CurrentTime();
        if (verbose_mode)
        {
          std::cout << "Timing Receiver is locked!" << std::endl;
          std::cout << "Current time is " << tr_formatDate(wrTime, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) <<std::endl; 
        }
      } 
      else
      { 
        std::cout << "Timing Receiver is NOT locked!" << std::endl;
        return (-1);
      }
      
      /* Search for outputs and inoutputs */
      std::map< std::string, std::string > outs;
      std::map< std::string, std::string > ins;
      std::string io_path;
      std::string io_partner;
      outs = receiver->getOutputs();
      ins = receiver->getInputs();
      
      /* Check if IO exists output */
      if (!just_inject)
      {
        for (std::map<std::string,std::string>::iterator it=outs.begin(); it!=outs.end(); ++it)
        {
          std::shared_ptr<Output_Proxy> output_proxy = Output_Proxy::create(it->second);
          if (verbose_mode) { std::cout << "Info: Found " << it->first << std::endl; }
          total_ios++;
          
          /* Set output enable if available */
          if (setup_io)
          {
            if (output_proxy->getOutputEnableAvailable())
            { 
              if (verbose_mode) { std::cout << "Turning output enable on... " << std::endl; }
              output_proxy->setOutputEnable(true);
            }
          }
          
          /* Turn off input termination if available */
          io_partner = output_proxy->getInput();
          if (io_partner != "") 
          {
            if (verbose_mode) { std::cout << "Found Partner Path: " << io_partner << std::endl; }
            if (setup_io)
            {
              std::shared_ptr<Input_Proxy> input_proxy = Input_Proxy::create(io_partner);
              if (input_proxy->getInputTerminationAvailable())
              { 
                if (verbose_mode) { std::cout << "Turning input termination off... " << std::endl; }
                input_proxy->setInputTermination(false);
              }
            }
          }
          
          /* Setup conditions */
          std::shared_ptr<OutputCondition_Proxy> condition_high = OutputCondition_Proxy::create(output_proxy->NewCondition(true, ECA_EVENT_ID, ECA_EVENT_MASK, 0,         true));
          std::shared_ptr<OutputCondition_Proxy> condition_low  = OutputCondition_Proxy::create(output_proxy->NewCondition(true, ECA_EVENT_ID, ECA_EVENT_MASK, 100000000, false));
          
          /* Accept all kinds of events */
          condition_high->setAcceptConflict(true);
          condition_high->setAcceptDelayed(true);
          condition_high->setAcceptEarly(true);
          condition_high->setAcceptLate(true);
          condition_low->setAcceptConflict(true);
          condition_low->setAcceptDelayed(true);
          condition_low->setAcceptEarly(true);
          condition_low->setAcceptLate(true);
        }
      }
      
      /* Output some information */
      std::cout << "ECA configuration done for " << total_ios << " IO(s)!" << std::endl;
      
      /* Create condition for the SCU bus (if wanted) */
      if (setup_scu_bus)
      {
        /* Search for SCU bus channel */
        map<std::string, std::string> e_scubusses = receiver->getInterfaces()["SCUbusActionSink"];
        if (e_scubusses.size() != 1)
        {
          std::cerr << "Device '" << receiver->getName() << "' has no SCU bus!" << std::endl;
          return (-1);
       }
       
       /* Get connection */
       std::shared_ptr<SCUbusActionSink_Proxy> e_scubus = SCUbusActionSink_Proxy::create(e_scubusses.begin()->second);
       std::shared_ptr<SCUbusCondition_Proxy> scubus_condition;
       scubus_condition = SCUbusCondition_Proxy::create(e_scubus->NewCondition(true, ECA_EVENT_ID, ECA_EVENT_MASK, 0, scu_bus_tag));
        
       /* Accept every kind of event */
       scubus_condition->setAcceptConflict(true);
       scubus_condition->setAcceptDelayed(true);
       scubus_condition->setAcceptEarly(true);
       scubus_condition->setAcceptLate(true);
       std::cout << "ECA configuration done for SCU bus!" << std::endl;
      }
      
      /* Trigger ECA continuously? */ 
      if (!external_trigger)
      {
        while(1)
        {
          /* Get time and align next PPS */
          wrTime = receiver->CurrentTime();
          if (verbose_mode) { std::cout << "Time (base):   0x" << std::hex << (UTC?wrTime.getUTC():wrTime.getTAI()) << std::dec << " -> " << tr_formatDate(wrTime, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) << std::endl; }
  
          /* Avoid late event and add one additional second */
          uint64_t nsec_fraction = wrTime.getTAI() % 1000000000;
          if (first_pps) { wrNext = (wrTime - nsec_fraction) + 2000000000; first_pps = false; }
          else           { wrNext = (wrTime - nsec_fraction) + 1000000000; }
          
          /* Print next pulse and inject event */
          std::cout << "Next pulse at: 0x" << std::hex << (UTC?wrNext.getUTC():wrNext.getTAI()) << std::dec << " -> " << tr_formatDate(wrNext, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) << std::endl;
          receiver->InjectEvent(ECA_EVENT_ID, 0x00000000, wrNext);
          
          /* Wait for the next pulse */
          while(wrNext>receiver->CurrentTime())
          { 
            /* Sleep 100ms to prevent too much Etherbone traffic */
            usleep(100000);
            
            /* Print time */
            if (verbose_mode) { 
              saftlib::Time time = receiver->CurrentTime();
              std::cout << "Time (wait):   0x" << std::hex << (UTC?time.getUTC():time.getTAI()) 
                        << std::dec << " -> " << tr_formatDate(time, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) 
                        << std::endl; 
            }
          }
        }
      }
      else
      {
        /* Setup SoftwareActionSink */
        std::cout << "Waiting for timing events..." << std::endl;
        std::shared_ptr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
        std::shared_ptr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(true, ECA_EVENT_ID, ECA_EVENT_MASK, 0));
        condition->SigAction = std::bind(&onAction, std::placeholders::_1//, .connect(sigc::bind(sigc::ptr_fun(&onAction), 0));
                                                  , std::placeholders::_2
                                                  , std::placeholders::_3
                                                  , std::placeholders::_4
                                                  , std::placeholders::_5
                                                  , 0, sink);
        
        /* Accept all kinds of events */
        condition->setAcceptConflict(true);
        condition->setAcceptDelayed(true);
        condition->setAcceptEarly(true);
        condition->setAcceptLate(true);
        


        
        /* Run the Glib event loop, inside callbacks you can still run all the methods like we did above */
        while (true) {
          saftlib::wait_for_signal();
        }
      }
    }
    catch (const saftbus::Error& error) 
    {
      /* Catch error(s) */
      std::cerr << "Failed to invoke method: " << error.what() << std::endl;
      return (-1);
    }
  }
  
  /* Done */
  return (0);
}
