// @file saft-ctl.cpp
// @brief Command-line interface for saftlib
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

static const char* program;

// static guint64 mask(int i) {
//  return i ? (((guint64)-1) << (64-i)) : 0;
// }


// format date string 
static const char* formatDate(guint64 time)
{
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  static char full[80];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns);

  return full;
} // format date


// this will be called, in case we are snooping for events
void on_action(guint64 id, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  std::cout << "Time: " << formatDate(deadline);
  std::cout << " ID: 0x"    << std::hex << std::setw(16) << std::setfill('0') << id;
  std::cout << " Param: 0x" << std::hex << std::setw(16) << std::setfill('0') << param;

  if (flags) {
    std::cout << "!";
    if (flags & 1) std::cout << "late";
    if (flags & 2) std::cout << "early";
    if (flags & 4) std::cout << "conflict";
    if (flags & 8) std::cout << "delayed";
  }
  std::cout << std::endl;
} 

using namespace saftlib;
using namespace std;


// display help
static void help(void) {
  std::cout << std::endl << "Usage: " << program << " <my unique name> [OPTIONS] [command]" << std::endl;
  std::cout << std::endl;
  std::cout << "  -h                   display this help and exit" << std::endl;
  std::cout << "  -s                   display actual status" << std::endl;
  std::cout << "  -i                   display info about local environment" << std::endl;
  std::cout << std::endl;
  std::cout << "  inject <eventID> <param> <time> inject event locally" << std::endl;
  std::cout << "  snoop  <eventID> <mask>         snoop events from DM, CTRL+C to exit (try 'snoop 0x0 0x0' for ALL)" << std::endl << std::endl;
  std::cout << "  attach <name> <path>            instruct saftd to control a new device (admin only)" << std::endl;
  std::cout << "  remove <name>                   remove the device from saftlib management (admin only)" << std::endl;
  std::cout << "  quit                            instructs the saftlib daemon to quit (admin only)" << std::endl << std::endl;
  std::cout << "Report saftlib bugs to <w.terpstra@gsi.de> !!!" << std::endl;
  std::cout << "Licensed under the GPL v3." << std::endl;
  std::cout << std::endl;
} // help


// display status
static void displayStatus(Glib::RefPtr<TimingReceiver_Proxy> receiver,
						  Glib::RefPtr<SoftwareActionSink_Proxy> sink) {
  guint32       nFreeConditions, nMax;
  bool          wrLocked;
  guint64       wrTime;
  guint32       capacity;

  map<Glib::ustring, Glib::ustring> allSinks;
  Glib::RefPtr<SoftwareActionSink_Proxy> aSink;
  map<Glib::ustring, Glib::ustring>::iterator i;
  vector<Glib::ustring>::iterator j;


  wrLocked        = receiver->getLocked();

  if (wrLocked) {
	wrTime        = receiver->ReadCurrentTime();

	std::cout << "WR locked, time: " << formatDate(wrTime) <<std::endl; 
  } 
  else std::cout << "no WR lock!!!" << std::endl; 

  nFreeConditions  = receiver->getFree();



  std::cout << "receiver free: " << nFreeConditions; 
  
  nMax            = sink->getMostFull();
  capacity        = sink->getCapacity();  
  
  std::cout << ", software sinks,  max (capacity of HW): " << nMax << "(" << capacity << ")" << std::endl;

  // find software sinks and display their status
  allSinks = receiver->getSoftwareActionSinks(); 
  if (allSinks.size() > 0) {
	std::cout << "sinks instantiated on this host: " << allSinks.size() << std::endl;
	// get status of each sink
	for (i = allSinks.begin(); i != allSinks.end(); i++) {
	   aSink= SoftwareActionSink_Proxy::create(i->second);
	  std::cout << "  " << i->second  << std::endl;
	  std::cout << "  -- actions: " << aSink->getActionCount() 
				<< ", delayed: "   << aSink->getDelayedCount()
				<< ", conflict: "  << aSink->getConflictCount()
				<< ", late: "      << aSink->getLateCount()
				<< ", early: "     << aSink->getEarlyCount()
				<< ", overflow: "  << aSink->getOverflowCount()
				<< std::endl;
	  // get all conditions for this sink
	  vector< Glib::ustring > allConditions = aSink->getAllConditions();
	  std::cout << "  -- conditions: " << allConditions.size() << std::endl;
	  for (j = allConditions.begin(); j != allConditions.end(); j++ ) {
		Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(*j);
		fprintf(stdout,	"  ---- ID: 0x%016" PRIx64 ", mask: 0x%016" PRIx64 ", offset: %09" PRIi64 " ns, active %d\n", 
				(uint64_t)condition->getID(), 
				(uint64_t)condition->getMask(),
				(int64_t)condition->getOffset(), 
				(int)condition->getActive()); /* how! */
	  } // for all conditions
	} // for all sinks 
  } // display all sinks
} // displayStatus 


// display information on host system environment
static void displayInfo(Glib::RefPtr<SAFTd_Proxy> saftd) {
  Glib::ustring sourceVersion;
  Glib::ustring buildInfo;
  Glib::ustring ebDevice;  
  Glib::ustring devName;
  map< Glib::ustring, Glib::ustring > allDevices;
  map<Glib::ustring, Glib::ustring>::iterator i;
  Glib::RefPtr<TimingReceiver_Proxy> aDevice;
                                                                     
  sourceVersion   = saftd->getSourceVersion();
  buildInfo       = saftd->getBuildInfo();
  //ebDevice        = receiver->getEtherbonePath();
  //devName         = receiver->getName();
  allDevices      = saftd->getDevices();

  std::cout << "source version                  : " << sourceVersion << std::endl;
  std::cout << "build info                      : " << buildInfo << std::endl;
  //std::cout << "etherbone device                : " << ebDevice << std::endl;
  //std::cout << "device name                     : " << devName << std::endl;
  std::cout << "devices attached on this host   : " << allDevices.size() << std::endl;
  for (i = allDevices.begin(); i != allDevices.end(); i++ ) {
	aDevice =  TimingReceiver_Proxy::create(allDevices.begin()->second);
	std::cout << "  device: " << i->second;
	std::cout << ", name: " << aDevice->getName();
	std::cout << ", path: " << aDevice->getEtherbonePath();
	std::cout << std::endl;
  } //for i
  

} // displayInfo


int main(int argc, char** argv)
{
  // variables and flags for command line parsing
  int           opt;
  bool 			eventSnoop   = false;
  bool 			statusDisp   = false;
  bool 			infoDisp     = false;
  bool 			eventInject  = false;
  bool          deviceAttach = false;
  bool          deviceRemove = false;
  bool          saftdQuit    = false;
  char          *value_end;

  // variables snoop event
  guint64 snoopID    = 0x0;
  guint64 snoopMask  = 0x0;

  // variables inject event 
  guint64 eventID    = 0x0;
  guint64 eventParam = 0x0;
  guint64 eventTime  = 0x0;

  // variables attach, remove
  char    *deviceName = NULL;
  char    *devicePath = NULL;
  
  
  const char *sinkName, *command;

  // parse for options
  program = argv[0];

  while ((opt = getopt(argc, argv, "p:sih")) != -1) {
	switch (opt) {
	case 's':
	  statusDisp = true;
	  break;
	case 'i':
	  infoDisp = true;
	   break;
    case 'p':
	  eventSnoop = true;
	  snoopID   = strtoull(optarg, &value_end, 0);
	  snoopMask = strtoull(optarg + 1, &value_end, 0);
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
	std::cerr << program << " expecting one non-optional argument: <my unique name>" << std::endl;
    help();
    return 1;
  }

  sinkName = argv[optind];

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
      eventTime     = strtoull(argv[optind+4], &value_end, 0);
	  //std::cout << std::hex << eventTime << std::endl; 
	  if (*value_end != 0) {
		std::cerr << program << ": invalid time -- " << argv[optind+4] << std::endl;
		return 1;
	  } // time
	} // "inject"

	else if (strcasecmp(command, "snoop") == 0) {
	  if (optind+4  != argc) {
		std::cerr << program << ": expecting exactly two arguments: snoop <eventID> <mask>" << std::endl;
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
	} // "snoop"

	else if (strcasecmp(command, "attach") == 0) {
	  if (optind+4  != argc) {
		std::cerr << program << ": expecting exactly two arguments: attach <name> <path>" << std::endl;
		return 1;
	  } 
	  deviceAttach = true;
      deviceName = argv[optind+2];
	  //std::cout << deviceName << std::endl; 
	  if (strlen(deviceName) == 0) {
		std::cerr << program << ": invalid name  -- " << argv[optind+2] << std::endl;
		return 1;
	  } // name
	  devicePath = argv[optind+3];
	  //std::cout << devicePath << std::endl; 
	  if (strlen(devicePath) == 0) {
		std::cerr << program << ": invalid path -- " << argv[optind+3] << std::endl;
		return 1;
	  } // path
	} // "attach"

	else if (strcasecmp(command, "remove") == 0) {
	  if (optind+3  != argc) {
		std::cerr << program << ": expecting exactly one argument: remove <name>" << std::endl;
		return 1;
	  } 
	  deviceRemove = true;
      deviceName = argv[optind+2];
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

    // do things requested by commands, saftd management first
	// attach device
	if (deviceAttach) {
	  saftd->AttachDevice(deviceName, devicePath);
	} // attach device

	// remove device
	if (deviceRemove) saftd->RemoveDevice(deviceName);

	// quit !!!
	if (saftdQuit) saftd->Quit();

	//do things requested by other commands
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices.begin()->second);
    Glib::RefPtr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(sinkName));

    // inject event
	if (eventInject) receiver->InjectEvent(eventID, eventParam, eventTime);

	// do things requested by options
	if (infoDisp) displayInfo(saftd);
	if (statusDisp) displayStatus(receiver, sink);

	// snoop
	if (eventSnoop) {
	  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
	  Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
	  // Accept all errors
	  condition->setAcceptLate(true);
	  condition->setAcceptEarly(true);
	  condition->setAcceptConflict(true);
	  condition->Action.connect(sigc::ptr_fun(&on_action));
	  condition->setActive(true);
	  loop->run();
	} // eventSnoop
	
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}
