// @file saft-gmt-check.cpp
// @brief Command-line interface for saftlib. This tool allows to check
// some functionalities of the General Machine Timing System (GMT).
// @author Dietrich Beck  <d.beck@gsi.de>
//
// Copyright (C) 2015 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// Check GMT
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
using namespace saftlib;

// global variables
static const char* program;
static guint32 pmode = PMODE_NONE;       // how data are printed (hex, dec, verbosity)
static guint64 messageCounterAll = 0;    // counts all messages received from DM
static guint64 messageCounterDiff = 0;   // counts all messages since last "counter info message" from DM
static guint64 messageCounterStart = 0;  // counter value of first "counter info message" from DM
static guint64 messageCounterLast = 0;   // counter value of last "counter info message" 
static guint64 overflowCounterAll = 0;   // counter for all overflows
static guint64 overflowCounterDiff = 0;  // counter for overflows since last "counter info message" 
static guint64 overflowCounterStart = 0; // counter of overflows at first "counter info message"
static guint64 overflowCounterLast = 0;  // counter of overflows at last "counter info message"


static guint64 lostMessagesAll = 0;     // number of all lost messages since first "counter info message"
static guint64 lostMessagesDiff = 0;    // number of last messages since last "counter info message"
static guint64 messageCounterID = 0x0FA62F9000000000; //EvtID for "message counter info message"
static bool    firstRun = true; 
static bool    onlyPrintLoss = false;
static guint64 overflow_cnt = 0;

static void on_overflow(guint64 count)
{
  overflow_cnt = count;
}

// this will be called, in case we are snooping for events
static void on_action(guint64 id, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  guint64 tmp;

  if (id == messageCounterID) {
    // >>>>>>> fix me (requires new GW for DM)
    tmp = (param << 32) | ((param  >> 32) & 0xffffffff); param = tmp;
    // <<<<<<< fix me (requires new GW for DM)

    // check for lost messages and overflows
    lostMessagesAll     = param - messageCounterStart - messageCounterAll; 
    lostMessagesDiff    = param - messageCounterLast - messageCounterDiff;
    overflowCounterAll  = overflow_cnt - overflowCounterStart;
    overflowCounterDiff = overflow_cnt - overflowCounterLast;

    // remember counter values 
    if (firstRun) {
      messageCounterStart  = param;
      overflowCounterStart = overflow_cnt;
    }
    messageCounterLast  = param;
    overflowCounterLast = overflow_cnt;

    //
    if (!firstRun) {
      if ((!onlyPrintLoss) || lostMessagesDiff) {
        std::cout << std::dec 
                  << "msg total (lost: sum / network / overflow): " << messageCounterAll 
                  << " (" << lostMessagesAll << " / " << lostMessagesAll - overflowCounterAll << " / " << overflowCounterAll
                  << "), msg diff (...): " << messageCounterDiff 
                  << " (" << lostMessagesDiff << " / " << lostMessagesDiff - overflowCounterDiff << " / " << overflowCounterDiff << ")";
        if (lostMessagesAll)   std::cout << ": LOSS!";
        if (lostMessagesDiff) std::cout << ": DIFFLOSS!";
        std::cout << std::endl;
      } //if !onlyPrintLoss
      
      messageCounterDiff = 0;
    } // if !firstRun
    firstRun = false;
  } // if id
  
  //increment counter values
  if (!firstRun) {
    messageCounterDiff++;
    messageCounterAll++;
  }

  if (pmode & PMODE_VERBOSE) std::cout << "MSG received: " <<  messageCounterAll 
                                       << ", valid: " << (int)(!firstRun) << std::hex 
                                       << ", ID: 0x" << id << std::dec << std::endl;

  // !!! Print overflow_cnt here

} // on_action

using namespace saftlib;
using namespace std;

// display help
static void help(void) {
  std::cout << std::endl << "Usage: " << program << " <device name> [OPTIONS] [command]" << std::endl;
  std::cout << std::endl;
  std::cout << "  -h                   display this help and exit" << std::endl;
  std::cout << "  -f                   use the first attached device (and ignore <device name>)" << std::endl;
  std::cout << "  -l                   only print output in case of lost messages" << std::endl;
  //std::cout << "  -d                   display values in dec format" << std::endl;
  //std::cout << "  -x                   display values in hex format" << std::endl;
  std::cout << "  -v                   more verbosity" << std::endl;
  std::cout << std::endl;
  std::cout << "  count                counts messages received from DM and compares with number of messages sent by DM" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl;
  std::cout <<  "'saft-gmt-check asfd -fl count'" << std::endl;
  std::cout << "   This will starting counting and compare the number of received messages ('get value')" << std::endl;
  std::cout << "   with the number of messages actually sent ('set value') by the DM. This example will use the " << std::endl;
  std::cout << "   first timing receiver available in the host system and print only to screen in case of lost" << std::endl;
  std::cout << "   messages. Remark: The 'set value' is transmitted from the DM via a dedicated message." << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <d.beck@gsi.de> !!!" << std::endl;
  std::cout << "Licensed under the GPL v3." << std::endl;
  std::cout << std::endl;
} // help


int main(int argc, char** argv)
{
  // variables and flags for command line parsing
  int  opt;
  bool count           = false;
  bool useFirstDev     = false;

  // variables snoop event
  guint64 snoopID     = 0x0;
  guint64 snoopMask   = 0x0;
  gint64  snoopOffset = 0x0;
  
  // variables attach, remove
  char    *deviceName = NULL;
  
  const char *command;

  pmode = PMODE_NONE;

  // parse for options
  program = argv[0];
  while ((opt = getopt(argc, argv, "dlxvhf")) != -1) {
    switch (opt) {
    case 'f' :
      useFirstDev = true;
      break;
    case 'l' :
      onlyPrintLoss = true;
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
    
    // "count" 
    if (strcasecmp(command, "count") == 0) {
      if (optind+2  != argc) {
        std::cerr << program << ": expecting exactly zero arguments: count :-)" << std::endl;
        return 1;
      } 
      count = true;
    } // "count"
	
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
    
    Glib::RefPtr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
    sink->OverflowCount.connect(sigc::ptr_fun(&on_overflow));
      
    // count
    if (count) {
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
    } // count messages
    
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}



