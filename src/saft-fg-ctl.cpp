/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */


#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SCUbusActionSink.h"
#include "interfaces/SCUbusCondition.h"
#include "interfaces/FunctionGenerator.h" 
#include "interfaces/MasterFunctionGenerator.h" 
#include "interfaces/FunctionGeneratorFirmware.h" 
#include "CommonFunctions.h"

using namespace saftlib;
using namespace std;

// global flag to switch between UTC and TAI representation
bool UTC = false;

// The parsed contents of the datafile given to the demo program
struct ParamSet {
  std::vector< int16_t > coeff_a;
  std::vector< int16_t > coeff_b;
  std::vector< int32_t > coeff_c;
  std::vector< unsigned char > step;
  std::vector< unsigned char > freq;
  std::vector< unsigned char > shift_a;
  std::vector< unsigned char > shift_b;
};

// Hand off the entire datafile to SAFTd
static bool fill(std::shared_ptr<FunctionGenerator_Proxy> gen, const ParamSet& params)
{
  return gen->AppendParameterSet(
    params.coeff_a,
    params.coeff_b,
    params.coeff_c,
    params.step,
    params.freq,
    params.shift_a,
    params.shift_b);
}

// Refill the function generator if the buffer fill gets low
static void on_refill(std::shared_ptr<FunctionGenerator_Proxy> gen, const ParamSet& params)
{
  while (fill(gen, params)) { }
}

static void on_armed(bool armed, std::shared_ptr<SCUbusActionSink_Proxy> scu, uint64_t tag)
{
  if (armed) {
    std::cout << "Generating StartTag" << std::endl;
    scu->InjectTag(tag);
  }
}

// Report when the function generator starts
static void on_start(saftlib::Time time)
{
  std::cout << "Function generator started at " << tr_formatDate(time, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) << std::endl;
}

// Report when the function generator stops
static void on_stop(saftlib::Time time, bool abort, bool hardwareMacroUnderflow, bool microControllerUnderflow, bool *continue_main_loop)
{
  *continue_main_loop = false;
  std::cout << "Function generator stopped at " << tr_formatDate(time, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) << std::endl;
  // was there an error?
  if (abort)
    std::cerr << "Fatal error: Abort was called!" << std::endl;
  if (hardwareMacroUnderflow)
    std::cerr << "Fatal error: hardwareMacroUnderflow!" << std::endl;
  if (microControllerUnderflow)
    std::cerr << "Fatal error: microControllerUnderflow!" << std::endl;

}

static void help(std::shared_ptr<SAFTd_Proxy> saftd)
{
  std::cerr << "Usage: saft-fg-ctl [OPTION] < wavedata.txt\n";
  std::cerr << "\n";
  std::cerr << "  -d <device>   saftlib timing receiver device name\n";
  std::cerr << "  -f <fg-name>  name of function generator on device\n";
  std::cerr << "  -t <tag>      tag to use when triggering function generator\n";
  std::cerr << "  -e <id>       generate tag when timing event id is received\n";
  std::cerr << "  -g            generate tag immediately\n";
  std::cerr << "  -r            repeat the waveform over and over forever\n";
  std::cerr << "  -sm           scan connected fg channels and create masterfg\n";
  std::cerr << "  -si           scan connected fg channels and create individual channels\n";
  std::cerr << "  -U            print time values in UTC instead of TAI\n";
  std::cerr << "  -h            print this message\n";
  std::cerr << "\n";
  std::cerr << "SAFTd version: " << std::flush;
  std::cerr << saftd->getSourceVersion() << "\n";
  std::cerr << saftd->getBuildInfo() << std::endl;
}

static void slow_warning(int sig)
{
  std::cerr << "warning: no input data received via stdin within 2s, run with '-h' for help. continuing to wait ..." << std::endl;
}

// throws if any channel is already owned
static bool nothing_owned(std::shared_ptr<TimingReceiver_Proxy> receiver)  
{
  try {
    // Get a list of function generators on the receiver
    map<std::string, std::string> fgs = receiver->getInterfaces()["FunctionGenerator"];
    if (!fgs.empty())
      for (auto itr = fgs.begin(); itr != fgs.end(); ++itr) {
        std::cerr << "trying to own fg " << itr->first << " " << itr->second << std::endl;
        auto fg = FunctionGenerator_Proxy::create(itr->second);
        fg->Own();
        fg->Disown();
      }
  } catch (saftbus::Error &e) {
    std::cerr << "fg channel: " << e.what() << std::endl;
    return false;
  }
  try {
    // Get a list of master function generators on the receiver
    map<std::string, std::string> mfgs = receiver->getInterfaces()["MasterFunctionGenerator"];
    if (!mfgs.empty())
      for (auto itr = mfgs.begin(); itr != mfgs.end(); ++itr) {
        auto mfg = MasterFunctionGenerator_Proxy::create(itr->second);
        std::cerr << "trying to own mfg " << itr->first << " " << itr->second << std::endl;
        mfg->Own();
        mfg->Disown();
      }
  } catch (saftbus::Error &e) {
    std::cerr << "masterfg: " << e.what() << std::endl;
    return false;
  }
  return true;
}


int main(int argc, char** argv)
{
  try {
    std::shared_ptr<SAFTd_Proxy> saftd = SAFTd_Proxy::create();
    
    // Options
    std::string device;
    std::string fg;
    uint32_t tag = 0xdeadbeef; // !!! fix me; use a safe default
    uint64_t event = 0;
    bool eventSet = false;
    bool repeat = false;
    bool generate = false;
    bool scan = false; 
    char i_or_m = 0;
    
    // Process command-line
    int opt, error = 0;
    while ((opt = getopt(argc, argv, "d:f:rght:e:s:U")) != -1) {
      switch (opt) {
        case 'd':
          device = optarg;
          break;
        case 'f':
          fg = optarg;
          break;
        case 'r':
          repeat = true;
          break;
        case 'g':
          generate = true;
          break;
        case 't':
          tag = strtoul(optarg, 0, 0);
          break;
        case 'e':
          event = strtoul(optarg, 0, 0);
          eventSet = true;
          break;
        case 's':
          scan = true;
          i_or_m = optarg[0];
          if (i_or_m != 'i' && i_or_m != 'm') {
            std::cerr << ": use -si or -sm" << std::endl;
            error = 1;
          }
          break;
        case 'U':
          UTC = true;
          break;
        case 'h':
          help(saftd);
          return 0;
        case ':':
        case '?':
          error = 1;
          break;
        default:
          std::cerr << argv[0] << ": bad getopt result" << std::endl;
          error = 1;
      }
    }
    
    if (error) {
      help(saftd);
      return 1;
    }
    
    if (optind != argc) {
      std::cerr << "Unexpected argument: " << argv[optind] << std::endl;
      help(saftd);
      return 1;
    }
    

    // Setup a warning on too slow data
    signal(SIGALRM, &slow_warning);
    alarm(2);
    
    // Read the data file from stdin ... maybe come up with a better format in the future !!!
    // Get a list of devices from the saftlib directory
    map<std::string, std::string> devices = saftd->getDevices();
    
    // Find the requested device
    std::shared_ptr<TimingReceiver_Proxy> receiver;
    if (device.empty()) {
      if (devices.empty()) {
        std::cerr << "No devices found" << std::endl;
        return 1;
      } else if (devices.size() != 1) {
        std::cerr << "More than one device; specify a device with '-d <device>'" << std::endl;
        error = 1;
      } else {
       receiver = TimingReceiver_Proxy::create(devices.begin()->second);
      }
    } else {
      if (devices.find(device) == devices.end()) {
        std::cerr << "Could not find device '" << device << "'; pick one that exists" << std::endl;
        error = 1;
      } else {
       receiver = TimingReceiver_Proxy::create(devices[device]);
      }
    }
    
    // List available devices
    if (error) {
      std::cerr << "Available devices:" << std::endl;
      for (map<std::string, std::string>::iterator i = devices.begin(); i != devices.end(); ++i)
        std::cerr << "  " << i->first << std::endl;
      return 1;
    }
    
    // Confirm this device is an SCU
    map<std::string, std::string> scus = receiver->getInterfaces()["SCUbusActionSink"];
    if (scus.size() != 1) {
      std::cerr << "Device '" << receiver->getName() << "' is not an SCU" << std::endl;
      return 1;
    }
    std::shared_ptr<SCUbusActionSink_Proxy> scu = SCUbusActionSink_Proxy::create(scus.begin()->second);


    // Scan for fg channels
    if (scan) {
      map<std::string, std::string> fg_fw = receiver->getInterfaces()["FunctionGeneratorFirmware"];
      if (fg_fw.empty()) {
        std::cerr << "No FunctionGenerator firmware found" << std::endl;
        return 1;
      }

      std::shared_ptr<FunctionGeneratorFirmware_Proxy> fg_firmware = FunctionGeneratorFirmware_Proxy::create(fg_fw.begin()->second);
      std::cout << "Scanning for fg-channels ... " << std::flush;
      if (nothing_owned(receiver)) {
        try {
          std::map<std::string, std::string> scanning_result;
          if (i_or_m == 'i') scanning_result = fg_firmware->ScanFgChannels();
          if (i_or_m == 'm') scanning_result = fg_firmware->ScanMasterFg();
          std::cout << "done, found " << std::endl;
          for (auto &pair: scanning_result) {
            std::cout << "  " << pair.first << " " << pair.second << std::endl;
          }
        } catch (saftbus::Error &e) {
          std::cerr << "could not scan: " << e.what() << std::endl;
        }
      }
      return 0;
    }

    
    // Get a list of function generators on the receiver
    map<std::string, std::string> fgs = receiver->getInterfaces()["FunctionGenerator"];
    
    // Find the target FunctionGenerator
    std::shared_ptr<FunctionGenerator_Proxy> gen;
    if (fg.empty()) {
      if (fgs.empty()) {
        std::cerr << "No function generators found" << std::endl;
        return 1;
      } else if (fgs.size() != 1) {
        std::cerr << "More than one function generator; specify one with '-f <function-generator>'" << std::endl;
        error = 1;
      } else {
        gen = FunctionGenerator_Proxy::create(fgs.begin()->second);
      }
    } else {
      if (fgs.find(fg) == fgs.end()) {
        std::cerr << "Could not find function generator '" << fg << "'; pick one that exists" << std::endl;
        error = 1;
      } else {
        gen = FunctionGenerator_Proxy::create(fgs[fg]);
      }
    }
    
    // List available function generators
    if (error) {
      std::cerr << "Available function generators:" << std::endl;
      for (map<std::string, std::string>::iterator i = fgs.begin(); i != fgs.end(); ++i)
        std::cerr << "  " << i->first << std::endl;
      return 1;
    }
    
    // Ok! Find all the devices is now out of the way. Let's get some work done.

    ParamSet params;
    int32_t a, la, b, lb, c, n, s, num;
    while((num = fscanf(stdin, "%d %d %d %d %d %d %d\n", &a, &la, &b, &lb, &c, &n, &s)) == 7) {
      // turn off warning
      if (params.coeff_a.empty()) alarm(0);
      
      params.coeff_a.push_back(a);
      params.coeff_b.push_back(b);
      params.coeff_c.push_back(c);
      params.step.push_back(n);
      params.freq.push_back(s);
      params.shift_a.push_back(la);
      params.shift_b.push_back(lb);
    }
    
    if (num != EOF || !feof(stdin)) {
      std::cerr << "warning: junk data at end of input file" << std::endl;
    }
    
    if (params.shift_b.empty()) {
      std::cerr << "Provided data file was empty" << std::endl;
      return 1;
    }
    
    // Claim the function generator for ourselves
    gen->Own();
    
    // Stop whatever the function generator was doing
    gen->Abort();
    
    // Wait until not Enabled
    while (gen->getEnabled()) saftlib::wait_for_signal();
    
    // Clear any old waveform data
    gen->Flush();
    
    bool continue_main_loop = true;
    // Watch for events on the function generator
    gen->SigStarted.connect(sigc::ptr_fun(&on_start));
    gen->SigStopped.connect(sigc::bind(sigc::ptr_fun(&on_stop), &continue_main_loop));
    
    // Load up the parameters, possibly repeating until full
    while (fill(gen, params) && repeat) { }
    
    // Repeat the waveform forever?
    if (repeat) {
      // listen to refill indicator
      gen->Refill.connect(sigc::bind(sigc::ptr_fun(&on_refill), gen, params));
    }
    
    // FYI, how much data is this?
    std::cout << "Loaded " << gen->ReadFillLevel() / 1000000.0 << "ms worth of waveform" << std::endl;
    
    // Watch for a timing event? => generate tag on event
    if (eventSet) scu->NewCondition(true, event, ~0, 0, tag);

    // Trigger the function generator ourselves?
    if (generate) gen->SigArmed.connect(sigc::bind(sigc::ptr_fun(&on_armed), scu, tag));

    // Ready to execute!
    gen->setStartTag(tag);
    gen->Arm();
    
    // Wait until not enabled / function generator is done
    // ... if we don't care to keep repeating the waveform, we could quit immediately;
    //     SAFTd has been properly configured to run autonomously at this point.
    // ... of course, then the user doesn't get the final error status message either
    while(continue_main_loop) {
      saftlib::wait_for_signal(100);
    }
    
    // Print summary
    std::cout << "Successful execution of " << gen->ReadExecutedParameterCount() << " polynomial tuples" << std::endl;

  } catch (const saftbus::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}
