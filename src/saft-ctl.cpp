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
#include "interfaces/iOwned.h"
#include "src/CommonFunctions.h"

using namespace std;

static const char* program;
static guint32 pmode = PMODE_NONE;    // how data are printed (hex, dec, verbosity)
bool absoluteTime = false;            // inject absolute time?

// this will be called, in case we are snooping for events
static void on_action(guint64 id, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  std::cout << "tDeadline: " << tr_formatDate(deadline, pmode);
  std::cout << tr_formatActionEvent(id, pmode);
  std::cout << tr_formatActionParam(param, 0xFFFFFFFF, pmode);
  std::cout << tr_formatActionFlags(flags, executed - deadline, pmode);
  std::cout << std::endl;
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
  std::cout << "  -k                   display gateware version (hardware)" << std::endl;
  std::cout << "  -s                   display actual status of software actions" << std::endl;
  std::cout << std::endl;
  std::cout << "  inject <eventID> <param> <time>  inject event locally, time [ns] is relative (see option -p for precise timing)" << std::endl;
  std::cout << "  snoop  <eventID> <mask> <offset> snoop events from DM, offset is in ns, CTRL+C to exit (try 'snoop 0x0 0x0 0' for ALL)" << std::endl;
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
static void displayStatus(Glib::RefPtr<TimingReceiver_Proxy> receiver,
						  Glib::RefPtr<SoftwareActionSink_Proxy> sink) {
  guint32       nFreeConditions;
  bool          wrLocked;
  guint64       wrTime;
  int           width;
  string        fmt;

  map<Glib::ustring, Glib::ustring> allSinks;
  Glib::RefPtr<SoftwareActionSink_Proxy> aSink;

  map<Glib::ustring, Glib::ustring>::iterator i;
  vector<Glib::ustring>::iterator j;

  // display White Rabbit status
  wrLocked        = receiver->getLocked();
  if (wrLocked) {
    wrTime        = receiver->ReadCurrentTime();
    if (absoluteTime) std::cout << "WR locked, time: " << wrTime <<std::endl;
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
      vector< Glib::ustring > allConditions = aSink->getAllConditions();
      std::cout << "  -- conditions: " << allConditions.size() << std::endl;
      for (j = allConditions.begin(); j != allConditions.end(); j++ ) {
        Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(*j);
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
static void displayInfoSW(Glib::RefPtr<SAFTd_Proxy> saftd) {
  Glib::ustring sourceVersion;
  Glib::ustring buildInfo;

  sourceVersion   = saftd->getSourceVersion();
  buildInfo       = saftd->getBuildInfo();

  std::cout << "saftlib source version                  : " << sourceVersion << std::endl;
  std::cout << "saftlib build info                      : " << buildInfo << std::endl;
} // displayInfoSW


// display information on the hardware environmet
static void displayInfoHW(Glib::RefPtr<SAFTd_Proxy> saftd) {
  Glib::ustring sourceVersion;
  Glib::ustring buildInfo;
  Glib::ustring ebDevice;
  Glib::ustring devName;
  map< Glib::ustring, Glib::ustring > allDevices;
  map<Glib::ustring, Glib::ustring>::iterator i;
  Glib::RefPtr<TimingReceiver_Proxy> aDevice;

  map< Glib::ustring, Glib::ustring > gatewareInfo;
  map<Glib::ustring, Glib::ustring>::iterator j;

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


static void displayInfoGW(Glib::RefPtr<TimingReceiver_Proxy> receiver)
{
  std::cout << receiver->getGatewareVersion() << std::endl;
} // displayInfoGW


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
  char *value_end;

  // variables snoop event
  guint64 snoopID     = 0x0;
  guint64 snoopMask   = 0x0;
  gint64  snoopOffset = 0x0;


  // variables inject event
  guint64 eventID     = 0x0;     // full 64 bit EventID contained in the timing message
  guint64 eventParam  = 0x0;     // full 64 bit parameter contained in the tming message
  guint64 eventTNext  = 0x0;     // time for next event (this value is added to the current time or the next PPS, see option -p
  guint64 eventTime   = 0x0;     // time for next event in PTP time
  guint64 ppsNext     = 0x0;     // time for next PPS
  guint64 wrTime      = 0x0;     // current WR time

  // variables attach, remove
  char    *deviceName = NULL;
  char    *devicePath = NULL;

  const char *command;

  pmode = PMODE_NONE;

  // parse for options
  program = argv[0];
  while ((opt = getopt(argc, argv, "dxsvapijkhf")) != -1) {
    switch (opt) {
    case 'f' :
      useFirstDev = true;
      break;
    case 's':
      statusDisp = true;
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
      eventSnoop = true;
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
      } // mask
    } // "snoop"

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
    Gio::init();
    Glib::RefPtr<SAFTd_Proxy> saftd = SAFTd_Proxy::create();

    // do display information that is INDEPENDANT of a specific device
    if (infoDispSW) displayInfoSW(saftd);
    if (infoDispHW) displayInfoHW(saftd);

    // do things that DEPEND on a specific device

    // do commands for saftd management first
    // attach device
    if (deviceAttach) {
      saftd->AttachDevice(deviceName, devicePath);
    } // attach device

    // remove device
    if (deviceRemove) saftd->RemoveDevice(deviceName);

    // quit !!!
    if (saftdQuit) saftd->Quit();

    // get a specific device
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver;
    switch (useFirstDev) {
    case true  :
      receiver = TimingReceiver_Proxy::create(devices.begin()->second);
      break;
    case false :
      if (devices.find(deviceName) == devices.end()) {
        std::cerr << "Device '" << deviceName << "' does not exist" << std::endl;
        return -1;
      } // find device
      receiver = TimingReceiver_Proxy::create(devices[deviceName]);
      break;
    default :
      return 1;
    } //switch useFirstDevice;

    if (infoDispGW) displayInfoGW(receiver);


    Glib::RefPtr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));

    // display status of software actions
    if (statusDisp) displayStatus(receiver, sink);

    // inject event
    if (eventInject) {
      wrTime    = receiver->ReadCurrentTime();
      if (ppsAlign) {
        ppsNext   = (wrTime - (wrTime % 1000000000)) + 1000000000;
        eventTime = (ppsNext + eventTNext); }
      else if (absoluteTime) {
        eventTime = eventTNext;
      } // ppsAlign
      else eventTime = wrTime + eventTNext;

      receiver->InjectEvent(eventID, eventParam, eventTime);
      if (pmode & PMODE_HEX)
      {
        std::cout << "Injected event (eventID/parameter/time): 0x" << std::hex << std::setw(16) << std::setfill('0') << eventID
                                                                      << " 0x" << std::setw(16) << std::setfill('0') << eventParam
                                                                      << " 0x" << std::setw(16) << std::setfill('0') << eventTime << std::dec << std::endl;
      }

    } //inject event

    // snoop
    if (eventSnoop) {
      Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
      Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, snoopOffset));
      // Accept all errors
      condition->setAcceptLate(true);
      condition->setAcceptEarly(true);
      condition->setAcceptConflict(true);
      condition->setAcceptDelayed(true);
      condition->Action.connect(sigc::ptr_fun(&on_action));
      condition->setActive(true);
      loop->run();
    } // eventSnoop

  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}
