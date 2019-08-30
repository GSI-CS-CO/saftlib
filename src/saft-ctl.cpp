// @file saft-ctl.cpp
// @brief Command-line interface for saftlib. This tool focuses on the software part.
// @author Dietrich Beck  <d.beck@gsi.de>
//
// Copyright (C) 2015 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
bool absoluteTime   = false;
bool UTC            = false;          // show UTC instead of TAI
bool UTCleap        = false;

// GID
#define QR       0x1c0                // 'PZ1, Quelle Rechts'
#define QL       0x1c1                // 'PZ2, PQuelle Links'
#define QN       0x1c2                // 'PZ3, Quelle Hochladungsinjektor UN (HLI)'
#define HLI      0x1c3                // 'PZ4, Hochladungsinjektor UN (HLI)'
#define HSI      0x1c4                // 'PZ5, Hochstrominjektor UH (HSI)'
#define AT       0x1c5                // 'PZ6, Alvarez'
#define TK       0x1c6                // 'PZ7, Transferkanal'

// EVTNO
#define NXTACC   0x10                 // EVT_PREP_NEXT_ACC
#define NXTRF    0x12                 // EVT_RF_PREP_NXT_ACC

#define NPZ      7                    // # of UNILAC 'Pulszentralen'
#define FID      0x1

// this will be called, in case we are snooping for events
static void on_action(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
  std::cout << "tDeadline: " << tr_formatDate(deadline, pmode);
  std::cout << tr_formatActionEvent(id, pmode);
  std::cout << tr_formatActionParam(param, 0xFFFFFFFF, pmode);
  std::cout << tr_formatActionFlags(flags, executed - deadline, pmode);
  std::cout << std::endl;
} // on_action

// this will be called, in case we are snooping for events
static void on_action_uni(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
  uint32_t gid;
  uint32_t evtNo;
  uint32_t vacc;
  uint32_t flagsSPZ;
  uint32_t flagsPZ;
  string   sVacc;
  string   special;

  uint32_t      flagNochop;
  uint32_t      flagShortchop;
  uint32_t      flagHighC;
  uint32_t      flagRigid;
  uint32_t      flagDry;

  static std::string   pz1, pz2, pz3, pz4, pz5, pz6, pz7;
  static saftlib::Time prevDeadline = deadline;
  static uint32_t      nCycle       = 0x0;
  
  gid      = ((id    & 0x0fff000000000000) >> 48);
  evtNo    = ((id    & 0x0000fff000000000) >> 36);
  vacc     = ((id    & 0x00000000fff00000) >> 20);
  flagsSPZ = ((param & 0xffffffff00000000) >> 32);
  flagsPZ  = ( param & 0x00000000ffffffff);

  flagNochop    = ((flagsSPZ & 0x1) != 0);
  flagShortchop = ((flagsSPZ & 0x2) != 0);
  flagRigid     = ((flagsPZ  & 0x2) != 0);
  flagDry       = ((flagsPZ  & 0x4) != 0);
  flagHighC     = ((flagsPZ  & 0x8) != 0);

  if ((deadline - prevDeadline) > 10000000) { // new UNILAC cycle starts if diff > 10ms
    switch (nCycle) {
      case 0 :        // print header
        std::cout << "   # cycle:      QR      QL      QN     HLI     HSI      AT      TK" << std::endl;
        break;
      case 1 ... 20 : // hack: throw away first cycles (as it takes a while to create the ECA conditions)
        break;
      default :       // default
                          
        std::cout << std::setw(10) << nCycle << ":"
                  << std::setw( 8) << pz1
                  << std::setw( 8) << pz2
                  << std::setw( 8) << pz3
                  << std::setw( 8) << pz4
                  << std::setw( 8) << pz5
                  << std::setw( 8) << pz6 
                  << std::setw( 8) << pz7
                  << std::endl;
        break;
    } // switch nCycle
    pz1 = pz2 = pz3 = pz4 = pz5 = pz6 = pz7 = "";
    prevDeadline  = deadline;
    nCycle++;
  } // if deadline

  sVacc = "";
  // special cases
  if (evtNo == NXTRF) sVacc = "(HF)";
  if (evtNo == NXTACC) {
    if (((gid == QR) || (gid == QL) || (gid == QN)) && (vacc == 14)) sVacc = "(IQ)";
    else {
      special = "";
      if (flagNochop)    special  = "N";
      if (flagShortchop) special += "S";
      if (flagHighC)     special += "H";
      if (flagRigid)     special += "R"; 
      if (flagDry)       special += "D"; 
      sVacc = special + std::to_string(vacc);
    } // else: ion source producing real beam
  } // if NXTACC

  switch (gid) {
  case QR : 
    pz1 = sVacc;
    break;
  case QL :
    pz2 = sVacc;
    break;
  case QN :
    pz3 = sVacc;
    break;
  case HLI :
    pz4 = sVacc;
    break;
  case HSI :
    pz5 = sVacc;
    break;
  case AT :
    pz6 = sVacc;
    break;
  case TK :
    pz7 = sVacc;
    break;
  default :
    break;
  } // switch gid
} // on_action_uni

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
  std::cout << "  -k                   display gateware version (hardware)" << std::endl;
  std::cout << "  -s                   display actual status of software actions" << std::endl;
  std::cout << "  -t                   display the current temperature in Celsius (if sensor is available) " << std::endl;
  std::cout << "  -U                   display/inject absolute time in UTC instead of TAI" << std::endl;
  std::cout << "  -L                   used with command 'inject' and -U: if injected UTC second is ambiguous choose the later one" << std::endl;
  std::cout << std::endl;
  std::cout << "  inject  <eventID> <param> <time>  inject event locally, time [ns] is relative (see option -p for precise timing)" << std::endl;
  std::cout << "  snoop   <eventID> <mask> <offset> snoop events from DM, offset is in ns, CTRL+C to exit (try 'snoop 0x0 0x0 0' for ALL)" << std::endl;
  std::cout << "  usnoop  <type>       (experimental feature) snoop events from WR-UNIPZ @ UNILAC,  <type> may be one of the following" << std::endl;
  std::cout << "            '0'        event display, but limited to GIDs of UNILAC and special event numbers" << std::endl;
  std::cout << "            '1'        shows virt acc executed at the seven PZs, similar to 'rsupcycle'" << std::endl;
  std::cout << "                       the virt acc is accompanied by flags 'N'o chopper, 'S'hort chopper, 'R'igid beam, 'D'ry and 'H'igh current," << std::endl;
  std::cout << "                       'warming pulses' are shown in brackets" << std::endl;
  std::cout << std::endl;
  std::cout << "  attach <path>                    instruct saftd to control a new device (admin only)" << std::endl;
  std::cout << "  remove                           remove the device from saftlib management (admin only)" << std::endl;
  std::cout << "  quit                             instructs the saftlib daemon to quit (admin only)" << std::endl << std::endl;
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
    if (absoluteTime) std::cout << "WR locked, time: " << tr_formatDate(wrTime, UTC?PMODE_UTC:PMODE_NONE) <<std::endl;
    else std::cout << "WR locked, time: " << tr_formatDate(wrTime, pmode) <<std::endl;
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
        std::cout << "  ---- " << tr_formatActionEvent(condition->getID(), pmode) //ID: "  	<< fmt << std::setw(width) << std::setfill('0') << condition->getID()
                  << ", mask: "     	<< fmt << std::setw(width) << std::setfill('0') << condition->getMask()
                  << ", offset: "   	<< fmt << std::setw(9)     << std::setfill('0') << condition->getOffset()
                  << ", active: "   	<< std::dec << condition->getActive()
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
  bool uniSnoop       = false;
  int  uniSnoopType   = 0;
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
  

  // variables inject event
  uint64_t eventID     = 0x0;     // full 64 bit EventID contained in the timing message
  uint64_t eventParam  = 0x0;     // full 64 bit parameter contained in the tming message
  uint64_t eventTNext  = 0x0;     // time for next event (this value is added to the current time or the next PPS, see option -p
  saftlib::Time eventTime;     // time for next event in PTP time
  saftlib::Time ppsNext;     // time for next PPS 
  saftlib::Time wrTime;     // current WR time

  // variables attach, remove
  char    *deviceName = NULL;
  char    *devicePath = NULL;

  const char *command;

  pmode       = PMODE_NONE;

  // parse for options
  program = argv[0];
  while ((opt = getopt(argc, argv, "dxsvapijkhftUL")) != -1) {
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
      }
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
      if (optind+5  != argc) {
        std::cerr << program << ": expecting exactly three arguments: snoop <eventID> <mask> <offset>" << std::endl;
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
      
      eventSnoop = true;
    } // "snoop"

    else if (strcasecmp(command, "usnoop") == 0) {
      if (optind+3  != argc) {
        std::cerr << program << ": expecting exactly one argument: usnoop <type>" << std::endl;
        return 1;
      }
      uniSnoopType = strtoull(argv[optind+2], &value_end, 0);
      //std::cout << std::hex << snoopID << std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid type -- " << argv[optind+2] << std::endl;
        return 1;
      } // uniSnoopType
      if ((uniSnoopType < 0) || (uniSnoopType > 1)) {
        std::cerr << program << ": invalid value -- " << uniSnoopType << std::endl;
        return 1;
      } // range of uniSnoopType

      uniSnoop = true;
    } // "usnoop"

    else if (strcasecmp(command, "attach") == 0) {
      if (optind+3  != argc) {
        std::cerr << program << ": expecting exactly one argument: attach <path>" << std::endl;
        return 1;
      }
      deviceAttach = true;
      if (strlen(deviceName) == 0) {
        std::cerr << program << ": invalid name  -- " << argv[optind+2] << std::endl;
        return 1;
      } // name
      devicePath = argv[optind+2];
      std::cout << devicePath << std::endl;
      if (strlen(devicePath) == 0) {
        std::cerr << program << ": invalid path -- " << argv[optind+2] << std::endl;
        return 1;
      } // path
    } // "attach"

    else if (strcasecmp(command, "remove") == 0) {
      deviceRemove = true;
      std::cout << deviceName << std::endl;
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
      saftd->AttachDevice(deviceName, devicePath);
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
      std::thread t( [](){sleep(1);exit(1);} );
      saftd->Quit();
      t.join();
    }

    // get a specific device
    map<std::string, std::string> devices = SAFTd_Proxy::create()->getDevices();
    std::shared_ptr<TimingReceiver_Proxy> receiver;
    if (useFirstDev) {
      receiver = TimingReceiver_Proxy::create(devices.begin()->second);
    } else {
      if (devices.find(deviceName) == devices.end()) {
        std::cerr << "Device '" << deviceName << "' does not exist" << std::endl;
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
      while(true) {
        saftlib::wait_for_signal();
      }
    } // eventSnoop (without UNILAC option)

    // snoop for UNILAC
    if (uniSnoop) {
      snoopMask = 0xfffffff000000000;

      std::shared_ptr<SoftwareCondition_Proxy> condition[NPZ * 2];
      int nPz = 0;

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)QR << 48) | ((uint64_t)NXTACC << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)QR << 48) | ((uint64_t)NXTRF << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)QL << 48) | ((uint64_t)NXTACC << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)QL << 48) | ((uint64_t)NXTRF << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)QN << 48) | ((uint64_t)NXTACC << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)QN << 48) | ((uint64_t)NXTRF << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)HLI << 48 | ((uint64_t)NXTACC << 36));
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)HLI << 48) | ((uint64_t)NXTRF << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)HSI << 48 | ((uint64_t)NXTACC << 36));
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)HSI << 48) | ((uint64_t)NXTRF << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)AT << 48 | ((uint64_t)NXTACC << 36));
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)AT << 48) | ((uint64_t)NXTRF << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)TK << 48 | ((uint64_t)NXTACC << 36));
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)TK << 48) | ((uint64_t)NXTRF << 36);
      condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nPz++;

      //snoopID = ((uint64_t)FID << 60) | ((uint64_t)TK << 48 | ((uint64_t)NXTACC << 36));
      //condition[nPz] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      //nPz++;

      for (int i=0; i<nPz; i++) {
        condition[i]->setAcceptLate(true);
        condition[i]->setAcceptEarly(true);
        condition[i]->setAcceptConflict(true);
        condition[i]->setAcceptDelayed(true);
        switch (uniSnoopType) {
        case 1:
          condition[i]->SigAction.connect(sigc::ptr_fun(&on_action_uni));
          break;
        default : 
          condition[i]->SigAction.connect(sigc::ptr_fun(&on_action));
          break;
        }
        condition[i]->setActive(true);    
      } // for i

      while(true) {
        saftlib::wait_for_signal();
      }
    } // eventSnoop (with UNILAC option)

  } catch (const saftbus::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}

