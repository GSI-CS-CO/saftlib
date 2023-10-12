// @file saft-dm.cpp
// @brief Command-line interface for a local Data Master (dm). This tool
// allows to inject schedules with timing messages into the ECA. By this
// precisely timed actions can be generated by the ECA locally, if a
// central data master is not available on the timing network.
// @author Dietrich Beck  <d.beck@gsi.de>
//
//*  Copyright (C) 2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// Have your personal Data Master
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
#include <sstream>
#include <fstream>
#include <iomanip>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"
#include "interfaces/iDevice.h"
#include "interfaces/iOwned.h"
#include "CommonFunctions.h"

using namespace std;

static const char* program;

using namespace saftlib;
using namespace std;

// display help
static void help(void) {
  std::cout << std::endl << "Usage: " << program << " <device name> [OPTIONS] <file name>" << std::endl;
  std::cout << std::endl;
  std::cout << "  -h                   display this help and exit" << std::endl;
  std::cout << "  -f                   use the first attached device (and ignore <device name>)" << std::endl;
  std::cout << "  -p                   schedule will be added to next full second (option -p) or current time (option unused)" << std::endl;
  std::cout << "  -v                   print sleep time, inject time, scheduled time for each event" << std::endl;
  std::cout << std::endl;
  std::cout << "  -n N                 N (1..) is number of iterations of the schedule. If unspecified, N is set to 1" << std::endl;
  std::cout << std::endl;

  std::cout << "  The file must contain exactly one message per line. Each line must have the following format:" << std::endl;
  std::cout << "  '<eventID> <param> <time>', example: '0x1111000000000000 0x0 123000000', time [ns] and decimal." << std::endl;
  std::cout << std::endl;
  std::cout << "This tool provides a primitive Data Master for local operations in the FEC. Timing messages are injected" << std::endl;
  std::cout << "at the input of the ECA. This allows for scheduling actions in hard real-time down to ns. This tool might" << std::endl;
  std::cout << "be useful when a central Data Master is not available, for rapid prototyping or tests." << std::endl << std::endl;

  std::cout << "Report bugs to <d.beck@gsi.de> !!!" << std::endl;
  std::cout << "Licensed under the GPL v3." << std::endl;
  std::cout << std::endl;
} // help


int main(int argc, char** argv)
{
  // variables and flags for command line parsing
  int  opt;
  bool ppsAlign       = false;
  bool useFirstDev    = false;
  bool verboseMode    = false;
  unsigned int nIter  = 1;

  // variables inject event
  uint64_t eventID     = 0x0;     // full 64 bit EventID contained in the timing message
  uint64_t eventParam  = 0x0;     // full 64 bit parameter contained in the timing message
  uint64_t eventTime   = 0x0;     // time for event (this value is added to the current time or the next PPS, see option -p
  saftlib::Time startTime;        // time for start of schedule in PTP time
  saftlib::Time nextTimeWR;       // time for event (in units of WR time)
  int64_t  sleepTime   = 0x0;     // time interval for sleeping
  saftlib::Time wrTime;           // current WR time

  // variables attach, remove
  char    *deviceName = NULL;

  //helper variables
  string  line;
  unsigned int i;
  
  const char *filename;

  // parse for options
  program = argv[0];
  while ((opt = getopt(argc, argv, "n:phfv")) != -1) {
    switch (opt) {
	case 'n' :
	  nIter = atoi(optarg);
	  break;
    case 'f' :
      useFirstDev = true;
      break;
    case 'p':
      ppsAlign = true;
      break;
    case 'v':
      verboseMode = true;
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
  
  // parse for filename
  filename = "";
  if (optind + 1< argc) filename = argv[optind+1];
  else std::cerr << program << ": expecting non-optional argument <file name> " << std::endl;
  if (strlen(filename) == 0) std::cerr << program << ": illegal file name" << std::endl;

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
      if (devices.empty()) {
          std::cerr << "No devices attached to saftd" << std::endl;
          return -1;
      }
      receiver = TimingReceiver_Proxy::create(devices.begin()->second);
    } else {
      if (devices.find(deviceName) == devices.end()) {
        std::cerr << "Device '" << deviceName << "' does not exist" << std::endl;
        return -1;
      } // find device
      receiver = TimingReceiver_Proxy::create(devices[deviceName]);
    } //if useFirstDevice;

    // print headline
    uint8_t width = 53;
	std::cout << std::setfill(' ')
			<< std::setw(15) << "Event time"
			<< std::setw(19) << "Event ID"
			<< std::setw(19) << "Parameter";
	if (verboseMode) {
		std::cout << std::setfill(' ')
				<< std::setw(15) << "Sleep time"
				<< std::setw(20) << "Inject event"
				<< std::setw(20) << "Scheduled time";
		width = 108;
	}
	std::cout << std::endl << std::setfill('-') << std::setw(width) << "-" << std::endl;
	std::cout << std::setfill(' ');
	// Iterate the schedule nIter times.
	for (i=0; i<nIter; i++) {
	  wrTime    = receiver->CurrentTime();
	  if (ppsAlign) startTime = (wrTime - (wrTime.getTAI() % 1000000000)) + 1000000000;  //align schedule to next PPS
	  else          startTime = wrTime;                                         //align schedule to current WR time
	
	  ifstream myfile (filename);
	  if (myfile.is_open()) {
		while (getline (myfile,line)) {
		  // get data from file
		  std::stringstream stream(line);
		  stream >> std::hex >> eventID;
		  stream >> std::hex >> eventParam;
		  stream >> std::dec >> eventTime;
		  std::cout << std::dec << std::setfill(' ') << std::setw(15) << eventTime
				<< std::hex << " 0x" << std::setw(16) << eventID
				<< " 0x" << std::setfill('0') << std::setw(16) << eventParam;
		  // scheduled time for next inject
		  nextTimeWR = startTime + eventTime;
		  // sleep before injecting: keep the queue inside ECA short.
		  // lets sleep until 100ms before event is due - required to avoid overrun (when iterating) or late actions (for long schedules)
		  wrTime    = receiver->CurrentTime();
		  if (nextTimeWR > (wrTime + 100000000)) {                       //only sleep if injected event "was" more than 100ms in the future
			sleepTime = (int64_t)((nextTimeWR - wrTime) / 1000) - 100000; //sleep 100ms less than interval to injected event			
			usleep(sleepTime);
		  } // if next time
		  // set current time, only used for printing in verbose mode.
		  wrTime = receiver->CurrentTime();
		  // inject event
		  receiver->InjectEvent(eventID, eventParam, nextTimeWR);
		  if (verboseMode) {
			std::cout << std::dec << std::setfill(' ') << std::setw(15) << sleepTime * 1000
				  << " " << std::setw(19) << wrTime.getTAI()
				  << " " << std::setw(19) << nextTimeWR.getTAI();
		  }
		  std::cout << std::endl;
		  sleepTime = 0;
		} // while getline
		myfile.close();
	  }
	  else std::cerr << "Unable to open file" << std::endl; 
	} //for i
      
  } catch (const saftbus::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}



