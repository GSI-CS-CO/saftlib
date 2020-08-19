// @file saft-uni.cpp
// @brief Command-line interface for saftlib. This tool focuses on UNILAC specific features
// @author Dietrich Beck  <d.beck@gsi.de>
//
// Copyright (C) 2019 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// Have a chat with saftlib and UNILAC
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
// version: 2020-Aug-19

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
#define NVACC    16                   // # of vACC
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


// this will be called, in case we are snooping UNILAC cycles
static void on_action_uni_cycle(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
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
  if (evtNo == NXTRF) sVacc = "(HFW)";
  if (evtNo == NXTACC) {
    if (vacc == 14) {
      if ((gid == QR) || (gid == QL) || (gid == QN)) sVacc = "(IQ)";
      else                                           sVacc = "(HFC)";
    } // if vacc 14
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
} // on_action_uni_cycle


// this will be called, in case we are snooping for UNILAC virtual accelerators
static void on_action_uni_vacc(uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags)
{
#define OBSCYCLES      200                 // # of cycles observed before printing (a cycle takes 20ms)

  uint32_t gid;
  uint32_t vacc;
  uint32_t flagsSPZ;
  uint32_t flagsPZ;

  string   sVacc;
  string   rf;
  int      i, j;
  double   rate;
  
  static std::string   pz1, pz2, pz3, pz4, pz5, pz6, pz7;
  static saftlib::Time prevDeadline = deadline;
  static uint32_t nCycle            = 0x0;
  static uint32_t firstTime         = 0x1;
  static uint32_t nExe[NPZ][NVACC];                     // # of vacc executions within OBSCYCLES

  static uint32_t flagNochop[NVACC];
  static uint32_t flagShortchop[NVACC];
  static uint32_t flagHighC[NVACC];
  static uint32_t flagRigid[NVACC];
  static uint32_t flagDry[NVACC];

  if (firstTime) {                                     
    for (j=0; j<NVACC; j++) {                          // clear execution counter and flags 
      for (i=0; i<NPZ; i++) nExe[i][j] = 0;          
      flagNochop[j]    = 0;
      flagShortchop[j] = 0;
      flagHighC[j]     = 0;
      flagRigid[j]     = 0;
      flagDry[j]       = 0;
    } // for j
    firstTime = 0x0;
    std::cout << std::endl;
  } // if firstTime

  gid      = ((id & 0x0fff000000000000) >> 48);
  vacc     = ((id & 0x00000000fff00000) >> 20);
  flagsSPZ = ((param & 0xffffffff00000000) >> 32);
  flagsPZ  = ( param & 0x00000000ffffffff);

  if ((deadline - prevDeadline) > 10000000) {           // new UNILAC cycle starts if diff > 10ms
    if (((nCycle % OBSCYCLES) == 0) && (nCycle > 0)) {  // observation period is over
      std::cout << std::endl;
      std::cout << "vacc   QR   QL   QN   HLI  HSI  AT   TK  rate flags";
      std::cout << std::endl;
      for (j=0; j < NVACC; j++) {
        rate = 0.0;
        std::cout << std::setw(4) << j;
        for (i=0; i<NPZ; i++) {
          if (nExe[i][j] > 0) {
            std::cout    << "    X";
            rate = 50.0 * (double)(nExe[i][j])/(double)OBSCYCLES; // this assumes UNILAC runs at 50Hz which might not be true always
          } // if nExe
          else std::cout << "     ";
        } // for PZs
        if (rate > 0.0001) std::cout << std::setw(6) << fixed << setprecision(2) << rate; // printf was so easy :)
        std::cout << " ";
        if (flagNochop[j])    std::cout << "N";  
        if (flagShortchop[j]) std::cout << "S";
        if (flagHighC[j])     std::cout << "H";
        if (flagRigid[j])     std::cout << "R";
        if (flagDry[j])       std::cout << "D";
        std::cout << std::endl;
      } // for vacc
      
      std::cout << std::endl;
      for (j=0; j<NVACC; j++) {                         // clear execution counter and flags 
        for (i=0; i<NPZ; i++) nExe[i][j] = 0;          
        flagNochop[j]    = 0;
        flagShortchop[j] = 0;
        flagHighC[j]     = 0;
        flagRigid[j]     = 0;
        flagDry[j]       = 0;
      } // for j
    } // if nCycle % OBSCYCLES
    else if ((nCycle % 10) == 0) std::cout << "." << std::flush; // user entertainment

    prevDeadline = deadline;                           
    nCycle++;
  } // if deadline (new UNILAC cycle)

  if (vacc >= NVACC)            return;                // illegal vacc?
  if ((gid < QR) || (gid > TK)) return;                // illegal gid?

  (nExe[gid - QR][vacc])++;                            // increase run counter
  if ((flagsSPZ & 0x1) != 0) flagNochop[vacc]    = 1;     // set flags ...
  if ((flagsSPZ & 0x2) != 0) flagShortchop[vacc] = 1;
  if ((flagsPZ  & 0x2) != 0) flagRigid[vacc]     = 1;
  if ((flagsPZ  & 0x4) != 0) flagDry[vacc]     = 1;
  if ((flagsPZ  & 0x8) != 0) flagHighC[vacc]       = 1;

  return;
} // on_action_uni_vacc


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
  std::cout << "  snoop  <type>        snoop events from WR-UNIPZ @ UNILAC,  <type> may be one of the following" << std::endl;
  std::cout << "            '0'        event display, but limited to GIDs of UNILAC and a subset of event numbers" << std::endl;
  std::cout << "            '1'        shows virtual accelerator executed at the seven PZs, similar to 'rsupcycle'" << std::endl;
  std::cout << "            '2'        shows virtual accelerator executed at the seven PZs, similar to 'eOverview'" << std::endl;
  std::cout << std::endl;
  std::cout << "This tool snoops and diplays UNILAC specific info." <<std::endl;
  std::cout << std::endl;
  std::cout << "UNILAC is operated at 50Hz cycle rate. Timing sections are named QR, QL, HSI, QN, HLI, AT and TK." <<std::endl;
  std::cout << "There exist two injectors:" <<std::endl;
  std::cout << "1. The High Current Injector (HSI) with two ions sources (QR, QL)." <<std::endl;
  std::cout << "2. The High Charge Injector (HLI) with one ion source (QN)." <<std::endl;
  std::cout << "The beams of the two injectors are accelerated into the 'Alvarez Section' (AT). The beam can be" << std::endl;
  std::cout << "used for experiments or guided to the SIS18 via the 'Transfer Channel' (TK)." << std::endl;
  std::cout << std::endl;
  std::cout << "Beams are defined by 'Virtual Acceleratores' (vacc):" <<std::endl;
  std::cout << "vacc 0..13 are used for standard operaton." <<std::endl;
  std::cout << "vacc 14 is used for rf-conditioning or standalone ion-source operation." <<std::endl;
  std::cout << "vacc 15 is used for warming pulses." <<std::endl;
  std::cout << std::endl;    
  std::cout << "Shown are flags indicating special modes of operation: N(o chopper), S(hort chopper), R(igid beam)," << std::endl; 
  std::cout << "D(ry 'beam') and H(igh current beam); warming pulses are shown in brackets" << std::endl;
  std::cout << std::endl;
  std::cout << "Report bugs to <d.beck@gsi.de> !!!" << std::endl;
  std::cout << "Licensed under the GPL v3." << std::endl;
  std::cout << std::endl;
} // help


int main(int argc, char** argv)
{
  // variables and flags for command line parsing
  int  opt;
  bool uniSnoop       = false;
  int  uniSnoopType   = 0;
  bool useFirstDev    = false;
  char *value_end;

  // variables snoop event
  uint64_t snoopID     = 0x0;
  uint64_t snoopMask   = 0x0;

  char    *deviceName = NULL;

  const char *command;

  pmode       = PMODE_NONE;

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
        std::cerr << program << ": expecting exactly one argument: snoop <type>" << std::endl;
        return 1;
      }
      uniSnoopType = strtoull(argv[optind+2], &value_end, 0);
      //std::cout << std::hex << snoopID << std::endl;
      if (*value_end != 0) {
        std::cerr << program << ": invalid type -- " << argv[optind+2] << std::endl;
        return 1;
      } // uniSnoopType
      if ((uniSnoopType < 0) || (uniSnoopType > 2)) {
        std::cerr << program << ": invalid value -- " << uniSnoopType << std::endl;
        return 1;
      } // range of uniSnoopType

      uniSnoop = true;
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
          case 2:
            condition[i]->SigAction.connect(sigc::ptr_fun(&on_action_uni_vacc));
            break;
          case 1:
            condition[i]->SigAction.connect(sigc::ptr_fun(&on_action_uni_cycle));
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

