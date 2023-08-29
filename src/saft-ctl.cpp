// @file saft-ctl.cpp
// @brief Command-line interface for saftlib. This tool focuses on the software part.
// @author Dietrich Beck  <d.beck@gsi.de>
//
//*  Copyright (C) 2015 GSI Helmholtz Centre for Heavy Ion Research GmbH
//
// Have a chat with saftlib
//
//*****************************************************************************
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//*****************************************************************************
//

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <iostream>
#include <iomanip>
#include <thread>

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"
#include "interfaces/iDevice.h"
#include "interfaces/iOwned.h"
#include "CommonFunctions.h"

using namespace std;

static const char* program;
static uint32_t pmode = PMODE_NONE;   // how data are printed (hex, dec, verbosity)
bool printJSON      = false;          // display values in JSON format
bool absoluteTime   = false;
bool UTC            = false;          // show UTC instead of TAI
bool UTCleap        = false;

// this will be called, in case we are snooping for events
static void on_action(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
  if (printJSON)
  {
    std::cout << "{ ";
    std::cout << "\"Deadline\": \"" << tr_formatDate(deadline, pmode, printJSON) << "\",";
  }
  else
  {
    std::cout << "tDeadline: " << tr_formatDate(deadline, pmode, printJSON);
  }
  std::cout << tr_formatActionEvent(id, pmode, printJSON);
  std::cout << tr_formatActionParam(param, 0xFFFFFFFF, pmode, printJSON);
  std::cout << tr_formatActionFlags(flags, executed - deadline, pmode, printJSON);

  if (printJSON)
  {
    uint64_t time_tai = deadline.getTAI();
    std::cout << std::hex << std::setfill('0') << "\"EventRaw\": \"0x" << std::setw(16) << id << "\"" << ", ";
    std::cout << std::hex << std::setfill('0') << "\"ParameterRaw\": \"0x" << std::setw(16) << param << "\"" << ", ";
    std::cout << std::dec << "\"DeadlineRaw\": " << time_tai;
    std::cout << " }"<< std::endl;
  }
  else
  {
    std::cout << std::endl;
  }

} // on_action


using namespace saftlib;
using namespace std;

// display help
static void help(void) {
  std::cout << std::endl << "Usage: " << program << " <device name> [OPTIONS] [command]" << std::endl;
  std::cout << std::endl;
  std::cout << "  -h                   display this help and exit" << std::endl;
  std::cout << "  -a                   absolute time injection" << std::endl;
  std::cout << "  -f                   use the first attached device (and ignore <device name>)" << std::endl;
  std::cout << "  -d                   display values in dec format" << std::endl;
  std::cout << "  -x                   display values in hex format" << std::endl;
  std::cout << "  -v                   more verbosity, usefull with command 'snoop'" << std::endl;
  std::cout << "  -p                   used with command 'inject': <time> will be added to next full second (option -p) or current time (option unused)" << std::endl;
  std::cout << "  -i                   display saftlib info" << std::endl;
  std::cout << "  -j                   list all attached devices (hardware)" << std::endl;
  std::cout << "  -J                   display values in JSON format" << std::endl;
  std::cout << "  -k                   display gateware version (hardware)" << std::endl;
  std::cout << "  -s                   display actual status of software actions" << std::endl;
  std::cout << "  -t                   display the current temperature in Celsius (if sensor is available) " << std::endl;
  std::cout << "  -U                   display/inject absolute time in UTC instead of TAI" << std::endl;
  std::cout << "  -L                   used with command 'inject' and -U: if injected UTC second is ambiguous choose the later one" << std::endl;
  std::cout << std::endl;
  std::cout << "  inject  <eventID> <param> <time>  inject event locally, time [ns] is relative (see option -p for precise timing)" << std::endl;
  std::cout << "  snoop   <eventID> <mask> <offset> [<seconds>] snoop events from DM, offset is in ns, " << std::endl;
  std::cout << "                                   snoop for <seconds> or use CTRL+C to exit (try 'snoop 0x0 0x0 0' for ALL)" << std::endl;
  std::cout << std::endl;
  std::cout << "  attach <path> [<poll-iv>]        instruct saftd to control a new device. " << std::endl;
  std::cout << "                                   <poll-iv> is the polling interval for MSI on USB devices (default is 1 ms)." << std::endl;
  std::cout << "  remove                           remove the device from saftlib management " << std::endl;
  std::cout << "  quit                             instructs the saftlib daemon to quit " << std::endl << std::endl;
  std::cout << std::endl;
  std::cout << "This tool displays Timing Receiver and related saftlib status. It can also be used to list the ECA status for" << std::endl;
  std::cout << "software actions. Furthermore, one can do simple things with a Timing Receiver (snoop for events, inject messages)." <<std::endl;
  std::cout << std::endl;
  std::cout << "Tip: For using negative values with commands such as 'snoop', consider" << std::endl;
  std::cout << "using the special argument '--' to terminate option scanning." << std::endl << std::endl;

  std::cout << "Report bugs to <d.beck@gsi.de> !!!" << std::endl;
  std::cout << "Licensed under the GPL v3." << std::endl;
  std::cout << std::endl;
} // help


// display status
static void displayStatus(std::shared_ptr<TimingReceiver_Proxy> receiver,
                          std::shared_ptr<SoftwareActionSink_Proxy> sink) {
  uint32_t       nFreeConditions;
  bool          wrLocked;
  saftlib::Time   wrTime;
  int           width;
  string        fmt;

  map<std::string, std::string> allSinks;
  std::shared_ptr<SoftwareActionSink_Proxy> aSink;

  map<std::string, std::string>::iterator i;
  vector<std::string>::iterator j;

  // display White Rabbit status
  wrLocked        = receiver->getLocked();
  if (wrLocked) {
    wrTime        = receiver->CurrentTime();
    if (absoluteTime) std::cout << "WR locked, time: " << tr_formatDate(wrTime, UTC?PMODE_UTC:PMODE_NONE, printJSON) <<std::endl;
    else std::cout << "WR locked, time: " << tr_formatDate(wrTime, pmode, printJSON) <<std::endl;
  }
  else std::cout << "no WR lock!!!" << std::endl;

  // display ECA status
  nFreeConditions  = receiver->getFree();
  std::cout << "receiver free conditions: " << nFreeConditions;

  std::cout << ", max (capacity of HW): " << sink->getMostFull()
            << "(" << sink->getCapacity() << ")"
            << ", early threshold: " << sink->getEarlyThreshold() << " ns"
            << ", latency: " << sink->getLatency() << " ns"
            << std::endl;

  // find software sinks and display their status
  allSinks = receiver->getSoftwareActionSinks();
  if (allSinks.size() > 0) {
    std::cout << "sinks instantiated on this host: " << allSinks.size() << std::endl;
    // get status of each sink
    for (i = allSinks.begin(); i != allSinks.end(); i++) {
      aSink = SoftwareActionSink_Proxy::create(i->second);
      std::cout << "  " << i->second
                << " (minOffset: " << aSink->getMinOffset() << " ns"
                << ", maxOffset: " << aSink->getMaxOffset() << " ns)"
                << std::endl;
      std::cout << "  -- actions: " << aSink->getActionCount()
                << ", delayed: "    << aSink->getDelayedCount()
                << ", conflict: "   << aSink->getConflictCount()
                << ", late: "       << aSink->getLateCount()
                << ", early: "      << aSink->getEarlyCount()
                << ", overflow: "   << aSink->getOverflowCount()
                << " (max signalRate: " << 1.0 / ((double)aSink->getSignalRate() / 1000000000.0) << "Hz)"
                << std::endl;
      // get all conditions for this sink
      vector< std::string > allConditions = aSink->getAllConditions();
      std::cout << "  -- conditions: " << allConditions.size() << std::endl;
      for (j = allConditions.begin(); j != allConditions.end(); j++ ) {
        std::shared_ptr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(*j);
        if (pmode & 1) {std::cout << std::dec; width = 20; fmt = "0d";}
        else           {std::cout << std::hex; width = 16; fmt = "0x";}
        std::cout << "  ---- " << tr_formatActionEvent(condition->getID(), pmode, printJSON) //ID: "   << fmt << std::setw(width) << std::setfill('0') << condition->getID()
                  << ", mask: "         << fmt << std::setw(width) << std::setfill('0') << condition->getMask()
                  << ", offset: "       << fmt << std::setw(9)     << std::setfill('0') << condition->getOffset()
                  << ", active: "       << std::dec << condition->getActive()
                  << ", destructible: " << condition->getDestructible()
                  << ", owner: "        << condition->getOwner()
                  << std::endl;
      } // for all conditions
    } // for all sinks
  } // display all sinks
} // displayStatus


// display information on the software environmet
static void displayInfoSW(std::shared_ptr<SAFTd_Proxy> saftd) {
  std::string sourceVersion;
  std::string buildInfo;

  sourceVersion   = saftd->getSourceVersion();
  buildInfo       = saftd->getBuildInfo();

  std::cout << "saftlib source version                  : " << sourceVersion << std::endl;
  std::cout << "saftlib build info                      : " << buildInfo << std::endl;
} // displayInfoSW


// display information on the hardware environmet
static void displayInfoHW(std::shared_ptr<SAFTd_Proxy> saftd) {
  std::string sourceVersion;
  std::string buildInfo;
  std::string ebDevice;
  std::string devName;
  map< std::string, std::string > allDevices;
  map<std::string, std::string>::iterator i;
  std::shared_ptr<TimingReceiver_Proxy> aDevice;

  map< std::string, std::string > gatewareInfo;
  map<std::string, std::string>::iterator j;

  allDevices      = saftd->getDevices();

  std::cout << "devices attached on this host   : " << allDevices.size() << std::endl;
  for (i = allDevices.begin(); i != allDevices.end(); i++ ) {
    aDevice =  TimingReceiver_Proxy::create(i->second);
    std::cout << "  device: " << i->second;
    std::cout << ", name: " << aDevice->getName();
    std::cout << ", path: " << aDevice->getEtherbonePath();
    std::cout << ", gatewareVersion : " << aDevice->getGatewareVersion();
    std::cout << std::endl;
    gatewareInfo = aDevice->getGatewareInfo();
    std::cout << "  --gateware version info:" << std::endl;
    for (j = gatewareInfo.begin(); j != gatewareInfo.end(); j++) {
      std::cout << "  ---- " << j->second << std::endl;
    } // for j
    if (pmode & PMODE_VERBOSE) {
      std::map<std::string, std::map<std::string, std::string> > interfaces = aDevice->getInterfaces();
      for (auto &interface: interfaces) {
        std::cout << "Interface: " << interface.first << std::endl;
        for (auto &name_objpath: interface.second) {
          std::cout << "   " << std::setw(20) << name_objpath.first << " " << name_objpath.second << std::endl;
        }
      }
    }
    std::cout <<std::endl;
  } //for i
} // displayInfoHW


static void displayInfoGW(std::shared_ptr<TimingReceiver_Proxy> receiver)
{
  std::cout << receiver->getGatewareVersion() << std::endl;
} // displayInfoGW

static void displayCurrentTemperature(std::shared_ptr<TimingReceiver_Proxy> receiver)
{
  if (receiver->getTemperatureSensorAvail())
    std::cout << "current temperature (Celsius): " << receiver->CurrentTemperature() << std::endl;
  else
    std::cout << "no temperature sensor is available in this device!" << std::endl;
}

int main(int argc, char** argv)
{
  // variables and flags for command line parsing
  int  opt;
  bool eventSnoop     = false;
  bool statusDisp     = false;
  bool infoDispSW     = false;
  bool infoDispHW     = false;
  bool infoDispGW     = false;
  bool ppsAlign       = false;
  bool eventInject    = false;
  bool deviceAttach   = false;
  bool deviceRemove   = false;
  bool useFirstDev    = false;
  bool saftdQuit      = false;
  bool currentTemp    = false;
  char *value_end;

  // variables snoop event
  uint64_t snoopID     = 0x0;
  uint64_t snoopMask   = 0x0;
  int64_t  snoopOffset = 0x0;
  int64_t snoopSeconds = 0x7FFFFFFFFFFFFFFF; // maximum value

  // variables inject event
  uint64_t eventID     = 0x0;     // full 64 bit EventID contained in the timing message
  uint64_t eventParam  = 0x0;     // full 64 bit parameter contained in the timing message
  uint64_t eventTNext  = 0x0;     // time for next event (this value is added to the current time or the next PPS, see option -p
  saftlib::Time eventTime;     // time for next event in PTP time
  saftlib::Time ppsNext;     // time for next PPS
  saftlib::Time wrTime;     // current WR time

  // variables attach, remove
  char    *deviceName = NULL;
  char    *devicePath = NULL;
  int      devicePollIv = 1; // MSI polling interval in ms. only relevant if MSIs needs to be polled

  const char *command;

  pmode       = PMODE_NONE;

  // parse for options
  program = argv[0];
  while ((opt = getopt(argc, argv, "dxsvapijJkhftUL")) != -1) {
    switch (opt) {
      case 'f' :
        useFirstDev = true;
        break;
      case 's':
        statusDisp = true;
        break;
      case 't':
        currentTemp = true;
        break;
      case 'i':
        infoDispSW = true;
        break;
      case 'a':
        absoluteTime = true;
        break;
      case 'j':
        infoDispHW = true;
        break;
      case 'J':
        printJSON = true;
        break;
      case 'k':
        infoDispGW = true;
        break;
      case 'p':
        ppsAlign = true;
        break;
      case 'U':
        UTC = true;
        pmode = pmode + PMODE_UTC;
        break;
      case 'L':
        if (UTC) {
          UTCleap = true;
        } else {
          std::cerr << "-L only works with -U" << std::endl;
          return -1;
        } // else 'L'
        break;
      case 'd':
        pmode = pmode + PMODE_DEC;
        break;
      case 'x':
        pmode = pmode + PMODE_HEX;
        break;
      case 'v':
        pmode = pmode + PMODE_VERBOSE;
        break;
      case 'h':
        help();
        return 0;
      default:
        std::cerr << program << ": bad getopt result" << std::endl;
        return 1;
    } // switch opt
  }   // while opt

  if (optind >= argc) {
    std::cerr << program << " expecting one non-optional argument: <device name>" << std::endl;
    help();
    return 1;
  }

  deviceName = argv[optind];

  // parse for commands
  if (optind + 1< argc) {
    command = argv[optind+1];

    // "inject"
    if (strcasecmp(command, "inject") == 0) {
      if (optind+5  != argc) {
        std::cerr << program << ": expecting exactly three arguments: send <eventID> <param> <time>" << std::endl;
        return 1;
      }
      eventInject = true;
      eventID     = strtoull(argv[optind+2], &value_end, 0);
      //std::cout << std::hex << eventID << std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid eventID -- " << argv[optind+2] << std::endl;
        return 1;
      } // eventID
      eventParam     = strtoull(argv[optind+3], &value_end, 0);
      //std::cout << std::hex << eventParam << std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid param -- " << argv[optind+3] << std::endl;
        return 1;
      } // param
      eventTNext = strtoll(argv[optind+4], &value_end, 10);
      //std::cout << std::hex << eventTime << std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid time -- " << argv[optind+4] << std::endl;
        return 1;
      } // time
    } // "inject"

    else if (strcasecmp(command, "snoop") == 0) {
      if (optind+5  != argc && optind+6 != argc) {
        std::cerr << program << ": expecting three or four arguments: snoop <eventID> <mask> <offset> [<seconds>]" << std::endl;
        return 1;
      }
      snoopID     = strtoull(argv[optind+2], &value_end, 0);
      //std::cout << std::hex << snoopID << std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid eventID -- " << argv[optind+2] << std::endl;
        return 1;
      } // snoopID
      snoopMask     = strtoull(argv[optind+3], &value_end, 0);
      //std::cout << std::hex << snoopMask<< std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid mask -- " << argv[optind+3] << std::endl;
        return 1;
      } // mask
      snoopOffset   = strtoll(argv[optind+4], &value_end, 0);
      //std::cout << std::hex << snoopOffset<< std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid offset -- " << argv[optind+4] << std::endl;
        return 1;
      } // offset
      if (optind+6 == argc) {
        snoopSeconds = strtoll(argv[optind+5], &value_end, 0);
        if (*value_end != 0) {
          std::cerr << program << ": invalid seconds for snoop -- " << argv[optind+5] << std::endl;
          return 1;
        } // seconds
      }

      eventSnoop = true;
    } // "snoop"

    else if (strcasecmp(command, "attach") == 0) {
      if (optind+3 != argc && optind+4 != argc) {
        std::cerr << program << ": expecting one or two arguments: attach <path> [<poll-iv>]" << std::endl;
        return 1;
      }
      deviceAttach = true;
      if (strlen(deviceName) == 0) {
        std::cerr << program << ": invalid name  -- " << argv[optind+2] << std::endl;
        return 1;
      } // name
      devicePath = argv[optind+2];
      // std::cout << devicePath << std::endl;
      if (strlen(devicePath) == 0) {
        std::cerr << program << ": invalid path -- " << argv[optind+2] << std::endl;
        return 1;
      } // path
      if (optind+4 == argc) {
        devicePollIv = atoi(argv[optind+3]);
        if (devicePollIv <= 0) {
          std::cerr << program << ": invalid MSI polling interval -- " << argv[optind+3] << std::endl;
          return 1;
        }
      }
    } // "attach"

    else if (strcasecmp(command, "remove") == 0) {
      deviceRemove = true;
      if (strlen(deviceName) == 0) {
        std::cerr << program << ": invalid name  -- " << argv[optind+2] << std::endl;
        return 1;
      } // name
    } // "remove"

    else if (strcasecmp(command, "quit") == 0) {
      if (optind+2  != argc) {
        std::cerr << program << ": expecting no argument: quit" << std::endl;
        return 1;
      }
      saftdQuit = true;
    } // "quit"

    else std::cerr << program << ": unknown command: " << command << std::endl;
  } // commands

  // no parameters, no command: just display help and exit
  if ((optind == 1) && (argc == 1)) {
    help();
    return 0;
  }

  try {
    // initialize required stuff
    std::shared_ptr<SAFTd_Proxy> saftd = SAFTd_Proxy::create();

    // do display information that is INDEPENDANT of a specific device
    if (infoDispSW) {
      displayInfoSW(saftd);
    }
    if (infoDispHW) {
      displayInfoHW(saftd);
    }

    // do things that DEPEND on a specific device

    // do commands for saftd management first
    // attach device
    if (deviceAttach) {
      saftd->AttachDevice(deviceName, devicePath, devicePollIv);
    } // attach device

    // remove device
    if (deviceRemove) {
      saftd->RemoveDevice(deviceName);
    }

    // quit !!!
    if (saftdQuit) {
      // exit the program in a second thread, because the main
      // thread will be stuck waiting for the response from saftd
      // which will never be sent after calling the quit() method
      // std::thread t( [](){usleep(100000);exit(0);} );
      saftd->Quit();
      // t.join();
    }

    // get a specific device
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver;
    if (useFirstDev) {
      receiver = TimingReceiver_Proxy::create(devices.begin()->second);
    } else {
      if (devices.find(deviceName) == devices.end()) {
        if (deviceRemove) {
          std::cerr << "Device '" << deviceName << "' was removed" << std::endl;
        } else {
          std::cerr << "Device '" << deviceName << "' does not exist" << std::endl;
        }
        return -1;
      } // find device
      receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    } //if(useFirstDevice);

    if (infoDispGW) {
      displayInfoGW(receiver);
    }

    if (currentTemp) {
      displayCurrentTemperature(receiver);
    }

    std::shared_ptr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));

    // display status of software actions
    if (statusDisp) {
      displayStatus(receiver, sink);
    }

    // inject event
    if (eventInject) {
      wrTime    = receiver->CurrentTime();
      if (ppsAlign) {
        ppsNext   = (wrTime - (wrTime.getTAI() % 1000000000)) + 1000000000;
        eventTime = (ppsNext + eventTNext); }
      else if (absoluteTime) {
        if (UTC) {
          eventTime = saftlib::makeTimeUTC(eventTNext, UTCleap);
        } else {
          eventTime = saftlib::makeTimeTAI(eventTNext);
        }
      } // ppsAlign
      else eventTime = wrTime + eventTNext;

      receiver->InjectEvent(eventID, eventParam, eventTime);

      if (pmode & PMODE_HEX)
      {
        std::cout << "Injected event (eventID/parameter/time): 0x" << std::hex << std::setw(16) << std::setfill('0') << eventID
                                                                      << " 0x" << std::setw(16) << std::setfill('0') << eventParam
                                                                      << " 0x" << std::setw(16) << std::setfill('0') << (UTC?eventTime.getUTC():eventTime.getTAI()) << std::dec << std::endl;
      }

    } //inject event

    // snoop
    if (eventSnoop) {
      std::shared_ptr<SoftwareCondition_Proxy> condition
        = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, snoopOffset));
      // Accept all errors
      condition->setAcceptLate(true);
      condition->setAcceptEarly(true);
      condition->setAcceptConflict(true);
      condition->setAcceptDelayed(true);
      condition->SigAction.connect(sigc::ptr_fun(&on_action));
      condition->setActive(true);
      // set up new thread to snoop for the given number of seconds
      bool runSnoop = true;
      std::thread tSnoop( [snoopSeconds, &runSnoop]()
        {
          sleep(snoopSeconds);
          runSnoop = false;
        }
      );
      int64_t snoopMilliSeconds;
      if (snoopSeconds < (0x7FFFFFFFFFFFFFFF / 1000)) {
        snoopMilliSeconds = snoopSeconds * 1000;
      } else {
        // This is a workaround to avoid an overflow when calculating snoopSeconds * 1000.
        snoopMilliSeconds = snoopSeconds;
      }
      while(runSnoop) {
        saftlib::wait_for_signal(snoopMilliSeconds);
      }
      tSnoop.join();
    } // eventSnoop (without UNILAC option)

  } catch (const saftbus::Error& error) {
    std::string msg(error.what());
    if (saftdQuit && msg == "object path \"/de/gsi/saftlib\" not found") {
      std::cerr << "Quit SAFTd service" << std::endl;
    } else {
      std::cerr << "Failed to invoke method: \'" << error.what() << "\'" << std::endl;
    }
  }

  return 0;
}
