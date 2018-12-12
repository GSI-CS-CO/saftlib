// @file saft-lcd.cpp
// @brief Live Chain Dispaly. This tool uses saftlib for on-line snooping 
//        and display of beam production chains
// @author Dietrich Beck  <d.beck@gsi.de>
//
// Copyright (C) 2015 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// A CLI that allows to see what happens in the facility.
//
// version: 2018-Dec-11
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
#define SAFT_LCD_VERSION "0.0.4"

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

// FID
#define FID          0x1

// GID, SIS, ESR, HEST
#define GTK3MV4_TO_PLTKMH2 0x1f2
#define SIS18_RING         0x12c
#define GTS1MU1_TO_GTS3MU1 0x1f9
#define GTS1MU1_TO_GTE3MU1 0x1f5
#define GTS3MU1_TO_GHFSMU1 0x1fb
#define GHFSMU1_TO_GTS6MU1 0x1fd
#define GTS6MU1_TO_ESR     0x1f8
#define ESR_RING           0x154
#define ESR_TO_GTV2MU2     0x20c
#define GTH4MU2_TO_GTV1MU1 0x209

// GID, CRYRING
#define YRT1IN_TO_YRT1LQ1  0xc8
#define CRYRING_RING       0xd2

//GID, caves
#define TO_HHD             0x1fa
#define TO_HFS             0x1fc
#define TO_HHT             0x200
#define TO_HADES           0x203
#define TO_HTM             0x206
#define TO_HTC             0x20d
#define TO_HTD             0x212
#define TO_HTA             0x20f
#define TO_HTP             0x211  

// EVTNO
#define COMMAND            0xff
#define SEQ_START          0x101
#define UNI_TCREQ          0x15e

#define MAXRULES     32

using namespace std;

static const char* program;
static guint32  pmode = PMODE_NONE;  // how data are displayed
static int      getVersion = 0;      // print version?

static guint32  source;              // GID of source
static guint32  idleSource;          // GID played when BPC idle
static guint32  bpcAct;              // actual BPC 
static guint32  vaccAct;             // actual vAcc from UNILAC
static int      nCmd;                // counts command events for desired machine


// this will be called while snooping
static void on_action_op(guint64 id, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{
  guint32 gid;
  guint32 evtno;
  guint32 bpcid;
  guint32 vacc;
  guint32 dry;
  string  gName;

  gid     = ((id & 0x0fff000000000000) >> 48);
  evtno   = ((id & 0x0000fff000000000) >> 36);
  bpcid   = ((param >> 42) & 0b1111111111111111111111);  // in case of SEQ_START
  vacc    = (id & 0xf);                                  // in case of TC_REQ
  dry     = (id & 0x10);

  // machine names
  switch (gid) {
    // SIS, ESR, HEST
  case GTK3MV4_TO_PLTKMH2   : gName = "UNILAC";     break;
  case SIS18_RING           : gName = "SIS18";      break;
  case GTS1MU1_TO_GTS3MU1   : gName = "NE3";        break;
  case GTS3MU1_TO_GHFSMU1   : gName = "NE4";        break;
  case GTS1MU1_TO_GTE3MU1   : gName = "NE5";        break;
  case GHFSMU1_TO_GTS6MU1   : gName = "NE5";        break;
  case GTS6MU1_TO_ESR       : gName = "NE12";       break;
  case ESR_RING             : gName = "ESR";        break;
  case ESR_TO_GTV2MU2       : gName = "NE8";        break;
  case GTH4MU2_TO_GTV1MU1   : gName = "NE8";        break;
    // CRYRING
  case YRT1IN_TO_YRT1LQ1    : gName = "YRT1";       break;
  case CRYRING_RING         : gName = "CRYRING";    break;
    // caves
  case TO_HHD               : gName = "HHD";        break;
  case TO_HFS               : gName = "HFS";        break;
  case TO_HHT               : gName = "HHT";        break;
  case TO_HADES             : gName = "HADES";      break;
  case TO_HTM               : gName = "HTM";        break;
  case TO_HTC               : gName = "HTC";        break;
  case TO_HTD               : gName = "HTD";        break;
  case TO_HTA               : gName = "HTA";        break;       
  case TO_HTP               : gName = "HTP";        break;       
  default                   : gName = "N/A";        break;
  }

  switch (evtno) {
  case SEQ_START :
    if (gid == source) {

      // old BPC finished
      if ((vaccAct == 0xffffffff) && (source == GTK3MV4_TO_PLTKMH2)) std::cout << " [no vAcc requested] ";
      std::cout << std::endl;

      // start new BPC
      bpcAct  = bpcid;
      vaccAct = 0xffffffff;
      nCmd    = 0;
      std::cout << std::setw(7) << bpcid << ": " << gName;
    }  // if gid
    else {
      if (bpcid == bpcAct)             std::cout << "-" << gName;
      else {if (pmode & PMODE_VERBOSE) std::cout << "(" << gName << ")";}
    } // else
    break;
  case COMMAND : 
    if (gid == idleSource) {
      nCmd++;
      switch (nCmd) {
      case 180 : std::cout << std::endl << "   idle: " << gName; nCmd = 0; break; // idle since a long time ...
      default  : if (pmode & PMODE_VERBOSE) std::cout << std::endl << "(  idle: " << gName << ")"; break;
      } // switch nCmd
    } // if gid
    break; 
  case UNI_TCREQ :
    if (source == GTK3MV4_TO_PLTKMH2) {
      vaccAct = vacc;
      std::cout << " [" << vacc;
      if (dry) std::cout << "(dry)";
      std::cout <<  "] ";
    } // if beam from UNILAC
    break;
  default : break;
  } // switch evtno
  std::cout << std::flush;
} // on_action


using namespace saftlib;
using namespace std;

// display help
static void help(void) {
  std::cout << std::endl << "Usage: " << program << " <device name> [OPTIONS] [command]" << std::endl;
  std::cout << std::endl;
  std::cout << "  -h                   display this help and exit" << std::endl;
  std::cout << "  -e                   display version" << std::endl;
  std::cout << "  -f                   use the first attached device (and ignore <device name>)" << std::endl;
  std::cout << "  -v                   more verbosity, usefull with command 'snoop'" << std::endl;
  std::cout << std::endl;
  std::cout << "  snoop <source>       snoops and displays Beam Production Chains, CTRL+C to exit (try 'snoop 0'')" << std::endl;
  std::cout << "                       machine: 0 UNILAC(TK), 1, SIS, 2 ESR, 3 YRT1IN" << std::endl;
  std::cout << std::endl;
  std::cout << "This tool provides a very basic display of beam production chains in the facility." << std::endl;
  std::cout << std::endl;
  std::cout << "Example: 'saft-lcd bla -f snoop 0' snoops facility for BPCs with origin at UNILAC(TK)," << std::endl;
  std::cout << "         the following information is displayed" << std::endl;
  std::cout << "BPCID: source-target(s) [UNI vAcc]"              << std::endl;
  std::cout << "----------------------------------"              << std::endl;
  std::cout << "    6: UNILAC-SIS18-NE5-HADES [10]"              << std::endl;
  std::cout << "    '       '     '   '     '   '"               << std::endl;
  std::cout << "    '       '     '   '     '   '- # of virt acc requested from UNILAC" << std::endl;
  std::cout << "    '       '     '   '     '- BPC target"       << std::endl;
  std::cout << "    '       '     '   '- via"                    << std::endl;
  std::cout << "    '       '     '- via"                        << std::endl;
  std::cout << "    '       '- BPC origin"                       << std::endl;
  std::cout << "    '- BPCID"                                    << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <d.beck@gsi.de> !!!"              << std::endl;
  std::cout << "Licensed under the GPL v3" << std::endl;
  std::cout << std::endl;
} // help


int main(int argc, char** argv)
{
  // variables and flags for command line parsing
  int  opt;
  bool snoop          = false;
  bool useFirstDev    = false;
  char *value_end;

  // variables snoop event
  guint64 snoopID     = 0x0;
  guint64 snoopMask   = 0x0;
  guint32 dummy;

  char *deviceName = NULL;
  
  const char *command;

  pmode   = PMODE_NONE;

  // parse for options
  program = argv[0];
  while ((opt = getopt(argc, argv, "vehf")) != -1) {
    switch (opt) {
    case 'e' :
      getVersion = 1;
      break;
    case 'f' :
      useFirstDev = true;
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
  
  if (getVersion) std::cout << program << " version " << SAFT_LCD_VERSION << std::endl;

  // parse for commands
  if (optind + 1< argc) {
    command = argv[optind+1];
    

    if (strcasecmp(command, "snoop") == 0) {
      if (optind+3  != argc) {
        std::cerr << program << ": expecting exactly one argument: snoop <verbosity>" << std::endl;
        return 1;
      }  
      std::cout << program << ": snooping facility for BPCs with origin at ";
      snoop = true;
      dummy = strtoull(argv[optind+2], &value_end, 0);
      switch (dummy) {
      case 0 : source = GTK3MV4_TO_PLTKMH2; idleSource = SIS18_RING;   std::cout << "UNILAC(TK)"       ; break;
      case 1 : source = SIS18_RING;         idleSource = SIS18_RING;   std::cout << "SIS18"            ; break;
      case 2 : source = ESR_RING;           idleSource = ESR_RING;     std::cout << "ESR"              ; break;
      case 3 : source = YRT1IN_TO_YRT1LQ1;  idleSource = CRYRING_RING; std::cout << "CRYRING Injector" ; break;
      default: std::cerr << std::endl << program << ": invalid machine -- " << dummy << std::endl      ; return 1;
      } // switch dummy
      std::cout << std::endl << std::endl;
      if (pmode & PMODE_VERBOSE) std::cout << "verbose mode: idle patterns and other BPCs are shown in brackets" << std::endl;
      std::cout << std::endl;

      std::cout << "  BPCID: source-target(s) [UNI vAcc]" << std::endl;
      std::cout << "------------------------------------" << std::endl;
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
    

    // snoop
    if (snoop) {
      Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();

      snoopMask = 0xfffffff000000000;

      Glib::RefPtr<SoftwareCondition_Proxy> condition[32];
      int nCon = 0;

      // GID, SIS, ESR, HEST     
      snoopID   = ((guint64)FID << 60) | ((guint64)GTK3MV4_TO_PLTKMH2 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)SIS18_RING << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 8));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)SIS18_RING << 48) | ((guint64)UNI_TCREQ << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 100000000));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)SIS18_RING << 48) | ((guint64)COMMAND << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 8));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)GTS1MU1_TO_GTS3MU1 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 16));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)GTS1MU1_TO_GTE3MU1 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 16));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)GTS3MU1_TO_GHFSMU1 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 24));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)GHFSMU1_TO_GTS6MU1 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 32));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)GTS6MU1_TO_ESR << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 36));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)ESR_RING << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 40));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)ESR_RING << 48) | ((guint64)COMMAND << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 40));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)ESR_TO_GTV2MU2 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 48));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)GTH4MU2_TO_GTV1MU1 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 48));
      nCon++;

      // CRYRING 
      snoopID   = ((guint64)FID << 60) | ((guint64)YRT1IN_TO_YRT1LQ1 << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 0));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)CRYRING_RING << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 56));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)CRYRING_RING << 48) | ((guint64)COMMAND << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 56));
      nCon++;

      // caves
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HHD << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HFS << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;

      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HHT << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HADES << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HTM << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HTC << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HTD << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HTA << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      snoopID   = ((guint64)FID << 60) | ((guint64)TO_HTP << 48) | ((guint64)SEQ_START << 36);
      condition[nCon] = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, 64));
      nCon++;
      
      for (int i=0; i<nCon; i++) {
        condition[i]->setAcceptLate(true);
        condition[i]->setAcceptEarly(true);
        condition[i]->setAcceptConflict(true);
        condition[i]->setAcceptDelayed(true);
        condition[i]->Action.connect(sigc::ptr_fun(&on_action_op));
        condition[i]->setActive(true);    
      } // for i

      loop->run();
    } // opSnoop
    
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}



