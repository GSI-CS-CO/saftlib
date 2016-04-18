/* Synopsis */
/* ==================================================================================================== */
/* IO control application */

/* Defines */
/* ==================================================================================================== */
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#define ECA_EVENT_ID 0xffff000000000000 /* FID=MAX & GRPID=MAX */

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

/* Namespace */
/* ==================================================================================================== */
using namespace saftlib;
using namespace std;

/* Global */
/* ==================================================================================================== */
static const char *program    = NULL;  /* Name of the application */
static const char *deviceName = NULL;  /* Name of the device */
bool verbose_mode             = false; /* Print verbose output to output stream => -v */
guint64 overflow_counter      = 0;
guint64 action_counter        = 0;
guint64 late_counter          = 0;
guint64 early_counter         = 0;
guint64 conflict_counter      = 0;
guint64 delayed_counter       = 0;

/* Prototypes */
/* ==================================================================================================== */
static std::string formatDate(guint64 time);
void onAction(guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags, int rule);
void onOverflowCount(guint64 count);
void onActionCount(guint64 count);
void onLateCount(guint64 count);
void onEarlyCount(guint64 count);
void onConflictCount(guint64 count);
void onDelayedCount(guint64 count);
static void pps_help (void);

/* Function formatDate() */
/* ==================================================================================================== */
static std::string formatDate(guint64 time)
{
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  static char full[80];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns);

  return full;
}

/* Function onAction() */
/* ==================================================================================================== */
void onAction(guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags, int rule)
{
  std::cout << "Got event at: 0x" << std::hex << executed << " -> " << formatDate(executed) << std::endl;
  if (verbose_mode)
  {
    std::cout << "  Deadline (Diff.): 0x" << deadline << " (0x" << (deadline-executed) << ")" << std::endl;
    std::cout << "  Flags:            0x" << flags << std::endl;
    std::cout << "  Rule:             0x" << rule << std::endl;
    std::cout << std::dec;
    std::cout << "  Action Counter:   " << action_counter << std::endl;
    std::cout << "  Overflow Counter: " << overflow_counter << std::endl;
    std::cout << "  Late Counter:     " << late_counter << std::endl;
    std::cout << "  Early Counter:    " << early_counter << std::endl;
    std::cout << "  Conflict Counter: " << conflict_counter << std::endl;
    std::cout << "  Delayed Counter:  " << delayed_counter << std::endl;
  }
}

/* Generic counter functions */
/* ==================================================================================================== */
void onActionCount(guint64 count)   { action_counter++; }
void onOverflowCount(guint64 count) { overflow_counter++; }
void onLateCount(guint64 count)     { late_counter++; }
void onEarlyCount(guint64 count)    { early_counter++; }
void onConflictCount(guint64 count) { conflict_counter++; }
void onDelayedCount(guint64 count)  { delayed_counter++; }

/* Function pps_help() */
/* ==================================================================================================== */
static void pps_help (void)
{
  /* Print arguments and options */
  std::cout << "PPS-GEN for SAFTlib " << std::endl;
  std::cout << "Usage: " << program << " <unique device name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -s: Turn output enable on and input termination off" << std::endl;
  std::cout << "  -e: External event mode (ECA configuration only)" << std::endl;
  std::cout << "  -h: Print help (this message)" << std::endl;
  std::cout << "  -v: Switch to verbose mode" << std::endl;
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
  bool show_help        = false; /* Print help => -h */
  bool first_pps        = true;  /* Is this the first PPS output? */
  bool wrLocked         = false; /* Is the timing receiver locked? */
  guint64 wrTime        = 0;     /* Current time */
  guint64 wrNext        = 0;     /* Execution time for the next PPS */

  /* Get the application name */
  program = argv[0]; 
  
  /* Parse for options */
  while ((opt = getopt(argc, argv, ":sevh")) != -1)
  {
    switch (opt)
    {
      case 's': { setup_io         = true; break; }
      case 'e': { external_trigger = true; break; }
      case 'v': { verbose_mode     = true; break; }
      case 'h': { show_help        = true; break; }
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
    std::cout << "Incorrect non-optional arguments (expecting exactly the device name)..." << std::endl;
  }
  
  /* Check if help is needed, otherwise evaluate given arguments */
  if (show_help)
  { 
    pps_help();
  }
  else
  {
    /* Initialize saftlib components */
    Gio::init();
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    
    /* Try to setup all outputs */
    try
    {
      map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
      if (devices.find(deviceName) == devices.end())
      {
        std::cerr << "Device '" << deviceName << "' does not exist!" << std::endl;
        return (-1);
      }
      Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
      
      /* Check if timing receiver is locked */
      wrLocked = receiver->getLocked();
      if (wrLocked)
      {
        wrTime  = receiver->ReadCurrentTime();
        if (verbose_mode)
        {
          std::cout << "Timing Receiver is locked!" << std::endl;
          std::cout << "Current time is " << formatDate(wrTime) <<std::endl; 
        }
      } 
      else
      { 
        std::cout << "Timing Receiver is NOT locked!" << std::endl;
        return (-1);
      }
      
      /* Search for outputs and inoutputs */
      std::map< Glib::ustring, Glib::ustring > outs;
      std::map< Glib::ustring, Glib::ustring > ins;
      Glib::ustring io_path;
      Glib::ustring io_partner;
      outs = receiver->getOutputs();
      ins = receiver->getInputs();
      Glib::RefPtr<Output_Proxy> output_proxy;
      Glib::RefPtr<Input_Proxy> input_proxy;
      
      /* Check if IO exists output */
      for (std::map<Glib::ustring,Glib::ustring>::iterator it=outs.begin(); it!=outs.end(); ++it)
      {
        output_proxy = Output_Proxy::create(it->second);
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
            input_proxy = Input_Proxy::create(io_partner);
            if (input_proxy->getInputTerminationAvailable())
            { 
              if (verbose_mode) { std::cout << "Turning input termination off... " << std::endl; }
              input_proxy->setInputTermination(false);
            }
          }
        }
        
        /* Setup conditions */
        output_proxy->NewCondition(true, ECA_EVENT_ID, 0xffff000000000000, 0,         true); 
        output_proxy->NewCondition(true, ECA_EVENT_ID, 0xffff000000000000, 100000000, false);
      }
      
      /* Output some information */
      std::cout << "ECA configuration done for " << total_ios << " IO(s)!" << std::endl;
      
      /* Trigger ECA continuously? */ 
      if (!external_trigger)
      {
        while(1)
        {
          /* Get time and align next PPS */
          wrTime = receiver->ReadCurrentTime();
          if (verbose_mode) { std::cout << "Time (base):   0x" << std::hex << wrTime << std::dec << " -> " << formatDate(wrTime) << std::endl; }
  
          /* Avoid late event and add one additional second */
          wrNext = wrTime % 1000000000;
          if (first_pps) { wrNext = (wrTime - wrNext) + 2000000000; first_pps = false; }
          else           { wrNext = (wrTime - wrNext) + 1000000000; }
          
          /* Print next pulse and inject event */
          std::cout << "Next pulse at: 0x" << std::hex << wrNext << std::dec << " -> " << formatDate(wrNext) << std::endl;
          receiver->InjectEvent(ECA_EVENT_ID, 0x00000000, wrNext);
          
          /* Wait for the next pulse */
          while(wrNext>receiver->ReadCurrentTime())
          { 
            if (verbose_mode) { std::cout << "Time (wait):   0x" << std::hex << receiver->ReadCurrentTime() << 
                                std::dec << " -> " << formatDate(receiver->ReadCurrentTime()) << std::endl; }
          }
        }
      }
      else
      {
        /* Setup SoftwareActionSink */
        std::cout << "Waiting for timing events..." << std::endl;
        Glib::RefPtr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
        Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(true, ECA_EVENT_ID, ECA_EVENT_ID, 0));
        condition->Action.connect(sigc::bind(sigc::ptr_fun(&onAction), 0));
        
        /* Attach to counter signals */
        sink->OverflowCount.connect(sigc::ptr_fun(&onOverflowCount));
        sink->ActionCount.connect(sigc::ptr_fun(&onActionCount));
        sink->LateCount.connect(sigc::ptr_fun(&onLateCount));
        sink->EarlyCount.connect(sigc::ptr_fun(&onEarlyCount));
        sink->ConflictCount.connect(sigc::ptr_fun(&onConflictCount));
        sink->DelayedCount.connect(sigc::ptr_fun(&onDelayedCount));
        
        /* Run the Glib event loop, inside callbacks you can still run all the methods like we did above */
        loop->run();
      }
    }
    catch (const Glib::Error& error) 
    {
      /* Catch error(s) */
      std::cerr << "Failed to invoke method: " << error.what() << std::endl;
      return (-1);
    }
  }
  
  /* Done */
  return (0);
}
