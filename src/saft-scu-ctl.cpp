/* Synopsis */
/* ==================================================================================================== */
/* SCU bus interface control application */

/* Defines */
/* ==================================================================================================== */
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

/* Includes */
/* ==================================================================================================== */
#include <stdio.h>
#include <iostream>
#include <giomm.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SCUbusActionSink.h"
#include "interfaces/SCUbusCondition.h"

#include "src/CommonFunctions.h"

/* Namespaces */
/* ==================================================================================================== */
using namespace saftlib;
using namespace std;

/* Globals */
/* ==================================================================================================== */
static const char *deviceName = NULL; /* Name of the device */
static const char *program    = NULL; /* Name of the application */

/* Prototypes */
/* ==================================================================================================== */
static void scu_help (void);

/* Function ecpu_help() */
/* ==================================================================================================== */
static void scu_help (void)
{
  /* Print arguments and options */
  std::cout << "SCU-CTL for SAFTlib" << std::endl;
  std::cout << "Usage: " << program << " <unique device name> [OPTIONS]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments/[OPTIONS]:" << std::endl;
  std::cout << "  -c <id> <mask> <offset> <tag>: Create a new condition" << std::endl;
  std::cout << "  -d:                            Disown the created condition" << std::endl;
  std::cout << "  -x:                            Destroy all unowned conditions" << std::endl;
  std::cout << "  -z                             Translate mask" << std::endl;
  std::cout << "  -l                             List conditions" << std::endl;
  std::cout << "  -h:                            Print help (this message)" << std::endl;
  std::cout << "  -v:                            Switch to verbose mode" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl;
  std::cout << program << " device " << "-c 64 58 0x2 0x4 -d"<< std::endl;
  std::cout << "  This will create a new condition and disown it" << std::endl;
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
  int  opt             = 0;
  char *pEnd           = NULL;
  bool create_sink     = false;
  bool disown_sink     = false;
  bool destroy_sink    = false;
  bool verbose_mode    = false;
  bool show_help       = false;
  bool translate_mask  = false;
  bool list_conditions = false;
  guint64 eventID      = 0x0;
  guint64 eventMask    = 0x0;
  gint64  offset       = 0x0;
  gint32  tag          = 0x0;
  Glib::ustring scu_name = "None";
  Glib::ustring scu_sink_name = "Unknown";
  
  /* Get the application name */
  program = argv[0]; 
  
  /* Parse arguments */
  while ((opt = getopt(argc, argv, "c:dxzlvh")) != -1)
  {
    switch (opt)
    {
      case 'c': 
      { 
        create_sink = true;
        if (argv[optind-1] != NULL) { eventID = strtoull(argv[optind-1], &pEnd, 0); }
        else                        { std::cerr << "Error: Missing event id!" << std::endl; return (-1); }
        if (argv[optind+0] != NULL) { eventMask = strtoull(argv[optind+0], &pEnd, 0); }
        else                        { std::cerr << "Error: Missing event mask!" << std::endl; return (-1); }
        if (argv[optind+1] != NULL) { offset = strtoull(argv[optind+1], &pEnd, 0);}
        else                        { std::cerr << "Error: Missing offset!" << std::endl; return (-1); }
        if (argv[optind+2] != NULL) { tag = strtoul(argv[optind+2], &pEnd, 0); }
        else                        { std::cerr << "Error: Missing tag!" << std::endl; return (-1); }
        break;
      }
      case 'd': { disown_sink     = true; break; }
      case 'x': { destroy_sink    = true; break; }
      case 'z': { translate_mask  = true; break; }
      case 'l': { list_conditions = true; break; }
      case 'v': { verbose_mode    = true; break; }
      case 'h': { show_help       = true; break; }
      default:  { std::cout << "Unknown argument..." << std::endl; show_help = true; break; }
    }
    /* Break loop if help is needed */
    if (show_help) { break; }
  }
  
  /* Plausibility check for arguments */
  if ((create_sink || disown_sink) && destroy_sink)
  {
    show_help = true;
    std::cerr << "Incorrect arguments!" << std::endl;
  }
  
  /* Does the user need help */
  if (show_help)
  {
    scu_help();
    return (-1);
  }
  
  /* List parameters */
  if (verbose_mode && create_sink)
  {
    std::cout << "Action sink/condition parameters:" << std::endl;
    std::cout << std::hex << "EventID:   0x" << eventID   << std::dec << " (" << eventID   << ")" << std::endl;
    std::cout << std::hex << "EventMask: 0x" << eventMask << std::dec << " (" << eventMask << ")" << std::endl;
    std::cout << std::hex << "Offset:    0x" << offset    << std::dec << " (" << offset    << ")" << std::endl;
    std::cout << std::hex << "Tag:       0x" << tag       << std::dec << " (" << tag       << ")" << std::endl;
  }
  
  /* Get the device name */
  deviceName = argv[optind];
  
  /* Initialize Glib stuff */
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  /* Try to connect to saftd */
  try 
  {
    /* Search for device name */
    if (deviceName == NULL)
    { 
      std::cerr << "Missing device name!" << std::endl;
      return (-1);
    }
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    if (devices.find(deviceName) == devices.end())
    {
      std::cerr << "Device '" << deviceName << "' does not exist!" << std::endl;
      return (-1);
    }
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    
    /* Search for embedded CPU channel */
    map<Glib::ustring, Glib::ustring> scus = receiver->getInterfaces()["SCUbusActionSink"];
    if (scus.size() != 1)
    {
      std::cerr << "Device '" << receiver->getName() << "' has no SCU bus" << std::endl;
      return (-1);
    }
    
    /* Get connection */
    Glib::RefPtr<SCUbusActionSink_Proxy> scu = SCUbusActionSink_Proxy::create(scus.begin()->second);
    
    /* Create the action sink now */
    if (create_sink)
    {
      /* Setup Condition */
      Glib::RefPtr<SCUbusCondition_Proxy> condition;
      if (translate_mask) { condition = SCUbusCondition_Proxy::create(scu->NewCondition(true, eventID, tr_mask(eventMask), offset, tag)); }
      else                { condition = SCUbusCondition_Proxy::create(scu->NewCondition(true, eventID, eventMask, offset, tag)); }
      
      /* Accept every kind of event */
      condition->setAcceptConflict(true);
      condition->setAcceptDelayed(true);
      condition->setAcceptEarly(true);
      condition->setAcceptLate(true);
      
      /* Run the Glib event loop in case the sink should not be disowned */
      if (disown_sink)
      {
        std::cout << "SCU bus Action sink configured and disowned..." << std::endl;
        condition->Disown();
        return (0);
      }
      else
      {
        std::cout << "SCU bus Action sink configured..." << std::endl;
        loop->run();
      }
    }
    else if (destroy_sink)
    {
      /* Get the conditions */
      std::vector< Glib::ustring > all_conditions = scu->getAllConditions();
      
      /* Destroy conditions if possible */
      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        Glib::RefPtr<SCUbusCondition_Proxy> destroy_condition = SCUbusCondition_Proxy::create(all_conditions[condition_it]);
        std::string cond_name = all_conditions[condition_it];
        if (destroy_condition->getDestructible() && (destroy_condition->getOwner() == ""))
        { 
          destroy_condition->Destroy();
          if (verbose_mode) { std::cout << "Destroyed " << cond_name << "!" << std::endl; }
        }
        else
        {
          if (verbose_mode) { std::cout << "Found " << cond_name << " but is not destructible!" << std::endl; }
        }
      }
    }
    else if (list_conditions)
    {
      /* Get the conditions */
      std::vector< Glib::ustring > all_conditions = scu->getAllConditions();
      
      /* List conditions */
      for (unsigned int condition_it = 0; condition_it < all_conditions.size(); condition_it++)
      {
        Glib::RefPtr<SCUbusCondition_Proxy> info_condition = SCUbusCondition_Proxy::create(all_conditions[condition_it]);
        std::string cond_name = all_conditions[condition_it];
        std::cout << cond_name << ":" << std::endl;
        std::cout << "  Event ID: 0x" << std::hex << info_condition->getID() << std::endl;
        std::cout << "  Mask:     0x" << std::hex << info_condition->getMask() << std::endl;
        std::cout << "  Tag:      0x" << std::hex << info_condition->getTag() << std::endl;
        std::cout << "  Offset:   " << std::dec << info_condition->getOffset() << std::endl;
        std::cout << "  Owner:    " << info_condition->getOwner() << std::endl;
        std::cout << std::endl;
      }
    }
    else
    {
      std::cerr << "Missing at least one parameter!" << std::endl;
      return (-1);
    }
    
  } 
  catch (const Glib::Error& error)
  {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  /* Done */
  return (0);
}
