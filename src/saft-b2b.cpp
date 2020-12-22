// @file saft-b2b.cpp
// @brief Command-line interface for saftlib. This tool focuses features specific for the bunch to bucket system
// @author Dietrich Beck  <d.beck@gsi.de>
//
// Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// Have a chat with saftlib and B2B
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
// version: 2020-Dec-21

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
static uint32_t pmode = PMODE_NONE;     // how data are printed (hex, dec, verbosity)
bool UTC            = false;            // show UTC instead of TAI
bool UTCleap        = false;
uint32_t reqSid;


// GID
#define GGSI        0x3a                // B2B prefix existing facility
#define SIS18       0x12c               // SIS18
#define ESR         0x154               // ESR

// EVTNO
#define KICKSTART1  0x031 
#define PMEXT 	    0x800
#define PMINJ 	    0x801
#define PREXT 	    0x802
#define PRINJ 	    0x803
#define TRIGGEREXT  0x804
#define TRIGGERINJ  0x805
#define DIAGMATCH   0x806
#define DIAGEXT     0x807
#define DIAGINJ     0x808
#define DIAGKICKEXT 0x809
#define DIAGKICKINJ 0x80a

#define FID         0x1

#define TUPDATE     500000000000        // interval for updating screen [ns]
uint64_t nextUpdate;                    // time for next update [ns]

uint32_t gid;                           // GID used for transfer
uint32_t sid;                           // SID user for transfer
uint32_t mode;                          // mode for transfer
double   fH1Ext;                        // h=1 frequency of extraction machine
uint32_t nHExt;                         // harmonic number of extraction machine 0..15
uint64_t TH1Inj;                        // h=1 period [as] of injection machine
uint32_t nHInj;                         // harmonic number of injection machine 0..15
uint64_t TBeat;                         // beating frquency
int32_t  cPhase;                        // correction for phase matching [ns]
int32_t  cTrigExt;                      // correction for extraction trigger
int32_t  cTrigInj;                      // correction for injection trigger

uint64_t tH1Ext;                        // h=1 phase  [ns] of extraction machine
uint64_t tH1Inj;                        // h=1 phase  [ns] of injection machine


// this will be called, in case we are snooping for events - dummy routine
static void on_action(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
  std::cout << "tDeadline: " << tr_formatDate(deadline, pmode);
  std::cout << tr_formatActionEvent(id, pmode);
  std::cout << tr_formatActionParam(param, 0xFFFFFFFF, pmode);
  std::cout << tr_formatActionFlags(flags, executed - deadline, pmode);
  std::cout << std::endl;
} // on_action


// clear all data
void clearAllData()
{
  gid      = 0x0;    
  sid      = 0x0;    
  mode     = 0x0;   
  fH1Ext   = 0x0;
  nHExt    = 0x0;  
  TH1Inj   = 0x0; 
  nHInj    = 0x0;  
  TBeat    = 0x0;  
  cPhase   = 0x0;
  cTrigExt = 0x0;
  cTrigInj = 0x0;
  tH1Ext   = 0x0; 
  tH1Inj   = 0x0;
} // clear all date

// print stuff to screen
void updateScreen(saftlib::Time deadline)
{
  int i;
  
  //clear screen
  for (i=0;i<60;i++) std::cout << std::endl;

  std::cout << "data: " << tr_formatDate(deadline, pmode); 
                           
                                                             
} // updateScreen


// this will be called, in case we are snooping B2B
static void on_action_sequence(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
  uint32_t recGid;
  uint32_t recEvtNo;
  uint32_t recSid;
  uint32_t TH1Ext;

  static uint32_t nCycle = 0x0;
  
  recGid      = ((id    & 0x0fff000000000000) >> 48);
  recEvtNo    = ((id    & 0x0000fff000000000) >> 36);
  recSid      = ((id    & 0x00000000fff00000) >> 20);

  if (recSid != sid) return;

  switch (recEvtNo) {
    case KICKSTART1 :
      clearAllData();
      break;
    case PMEXT :
      nHExt   = ((param & 0xff00000000000000) >> 56);
      TH1Ext  = ((param & 0x00ffffffffffffff));
      fH1Ext  = 
      break;
    default :
      ;
  } // switch recEvtNo
  
  if (deadline > nextUpdate) updateScreen();
} // on_action_sequence

using namespace saftlib;
using namespace std;

// display help
static void help(void) {
  std::cout << std::endl << "Usage: " << program << " <device name> [OPTIONS] [command]" << std::endl;
  std::cout << std::endl;
  std::cout << "  -h                   display this help and exit" << std::endl;
  std::cout << "  -f                   use the first attached device (and ignore <device name>)" << std::endl;
  std::cout << "  -d                   display values in dec format" << std::endl;
  std::cout << "  -x                   display values in hex format" << std::endl;
  std::cout << "  -v                   more verbosity, usefull with command 'snoop'" << std::endl;
  std::cout << "  -U                   display/inject absolute time in UTC instead of TAI" << std::endl;
  std::cout << "  -L                   used with command 'inject' and -U: if injected UTC second is ambiguous choose the later one" << std::endl;
  std::cout << std::endl;
  std::cout << "  snoop  <SID>         snoop events of the B2B system. Select SID of transfer" << std::endl;
  std::cout << std::endl;
  std::cout << "This tool snoops and diplays B2B specific info." <<std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <d.beck@gsi.de> !!!" << std::endl;
  std::cout << "Licensed under the GPL v3." << std::endl;
  std::cout << std::endl;
} // help


int main(int argc, char** argv)
{
  // variables and flags for command line parsing
  int  opt;
  bool b2bSnoop       = false;
  bool useFirstDev    = false;
  int  nCondition;
  char *value_end;

  // variables snoop event
  uint64_t snoopID     = 0x0;
  uint64_t snoopMask   = 0x0;

  char    *deviceName = NULL;

  const char *command;

  pmode       = PMODE_NONE;
  nextUpdate  = 0;

  // parse for options
  program = argv[0];
  while ((opt = getopt(argc, argv, "hdfULxv")) != -1) {
    switch (opt) {
      case 'f' :
        useFirstDev = true;
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
    
    if (strcasecmp(command, "snoop") == 0) {
      if (optind+3  != argc) {
        std::cerr << program << ": expecting exactly one argument: snoop <SID>>" << std::endl;
        return 1;
      }
      reqSid   = strtoull(argv[optind+2], &value_end, 0);
      b2bSnoop = true;
    } // "snoop"

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

    std::shared_ptr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));

    // snoop for B2B
    if (b2bSnoop) {

      nCondition = 3
      
      std::shared_ptr<SoftwareCondition_Proxy> condition[nCondition];

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)GGSI << 52);
      condition[0] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, 0xfff00000000000000, 0));
     
      snoopID = ((uint64_t)FID << 60) | ((uint64_t)SIS18 << 48);
      condition[1] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, 0xffff000000000000, 0));

      snoopID = ((uint64_t)FID << 60) | ((uint64_t)ESR << 48);
      condition[2] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, 0xffff000000000000, 0));

      for (int i=0; i<nCondition; i++) {
        condition[i]->setAcceptLate(true);
        condition[i]->setAcceptEarly(true);
        condition[i]->setAcceptConflict(true);
        condition[i]->setAcceptDelayed(true);
        condition[i]->SigAction.connect(sigc::ptr_fun(&on_action_sequece));
        condition[i]->setActive(true);    
      } // for i

      while(true) {
        saftlib::wait_for_signal();
      }
    } // eventSnoop

  } catch (const saftbus::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}

