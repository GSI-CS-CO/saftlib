/*  Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>

#include <SAFTd_Proxy.hpp>
#include <TimingReceiver_Proxy.hpp>
#include <SCUbusActionSink_Proxy.hpp>
#include <SCUbusCondition_Proxy.hpp>
#include <FunctionGenerator_Proxy.hpp>
#include <MasterFunctionGenerator_Proxy.hpp>
#include <CommonFunctions.hpp>

#include <saftbus/client.hpp>
#include <saftbus/error.hpp>


using namespace saftlib;
using namespace std;

/////////////////
// headers for profiling
#include <iostream>
#include <iomanip>
#include <sys/times.h>
#include <time.h>
#include <stdio.h>

// multithread test
#include <condition_variable>
#include <mutex>

#define MAX_CHANNELS 12
// 0 = silent, 1 = info, 2+ = debug
static const int loglevel=2;
static bool UTC = false; // use UTC instead of TAI for absolute time values

static timespec diff(timespec start, timespec end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

timespec time_start, time_end;

// Clock options:
#define CLOCK_ID CLOCK_MONOTONIC
//#define CLOCK_ID CLOCK_PROCESS_CPUTIME_ID
//#define CLOCK_ID CLOCK_REALTIME
	
static void timer_start()
{
	clock_gettime(CLOCK_ID, &time_start);
}
static void timer_stop()
{
	clock_gettime(CLOCK_ID, &time_end);
	std::cout<<diff(time_start,time_end).tv_sec<<"." << std::setfill('0') << std::setw(9) << diff(time_start,time_end).tv_nsec<<std::endl;
}

// thread signaling
static std::condition_variable all_stopped_cond_var;
static std::mutex fg_all_stopped_mutex;
static bool fg_all_stopped=false;
static std::condition_variable all_armed_cond_var;
static std::mutex fg_all_armed_mutex;
static bool fg_all_armed=false;

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

  
void test_master_fg(std::shared_ptr<SCUbusActionSink_Proxy> scu, std::shared_ptr<TimingReceiver_Proxy> receiver, ParamSet params, bool eventSet, uint64_t event, bool repeat, bool generate, uint32_t tag);


// Hand off the entire datafile to SAFTd
/*
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
*/

/*
static void on_armed(bool armed, std::shared_ptr<SCUbusActionSink_Proxy> scu, uint64_t tag)
{
  if (armed) {
  
    std::cout << "Generating StartTag in 2 seconds" << std::endl;
    sleep(2);
    scu->InjectTag(tag);
    std::cout << "Generating StartTag" << std::endl;
  }
}
*/

// Report when the function generator starts
/*
static void on_start(uint64_t time)
{
  std::cout << "Function generator started at " << format_time(time) << std::endl;
}
*/

// Report when the function generator stops
/*
static void on_stop(uint64_t time, bool abort, bool hardwareMacroUnderflow, bool microControllerUnderflow)
{
  std::cout << "Function generator stopped at " << format_time(time) << std::endl;
  // was there an error?
  if (abort)
    std::cerr << "Fatal error: Abort was called!" << std::endl;
  if (hardwareMacroUnderflow)
    std::cerr << "Fatal error: hardwareMacroUnderflow!" << std::endl;
  if (microControllerUnderflow)
    std::cerr << "Fatal error: microControllerUnderflow!" << std::endl;
}
*/

// Report on the individual function generators
static void on_master_started(std::string fg_name, saftlib::Time time)
{
  if (loglevel>0) std::cout << fg_name << " started at " << tr_formatDate(time, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) << std::endl;
}
static void on_master_armed(std::string fg_name, bool armed)
{
  if (loglevel>0) std::cout << fg_name << (armed ? " armed" : " disarmed")  << std::endl;
}
static void on_master_enabled(std::string fg_name, bool enabled)
{
  if (loglevel>0) std::cout << fg_name << (enabled ? " enabled" : " disabled")  << std::endl;
}
static void on_master_running(std::string fg_name, bool running)
{
  if (loglevel>0) std::cout << fg_name << (running ? " running" : " not running") << std::endl;
}
static void on_master_refill(std::string fg_name)
{
  if (loglevel>0) std::cout << fg_name << " refill request" << std::endl;
}
static void on_master_stop(std::string fg_name, saftlib::Time time, bool abort, bool hardwareMacroUnderflow, bool microControllerUnderflow)
{
  if (loglevel>0) std::cout << fg_name << " stopped at " << tr_formatDate(time, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) << std::endl;
  // was there an error?
  if (abort)
    std::cerr << "Fatal error: Abort was called!" << std::endl;
  if (hardwareMacroUnderflow)
    std::cerr << "Fatal error: hardwareMacroUnderflow!" << std::endl;
  if (microControllerUnderflow)
    std::cerr << "Fatal error: microControllerUnderflow!" << std::endl;
}

// report on the aggregated function generators
static void on_all_armed()
{    
  std::cout << "All armed." << std::endl;
  {
    std::lock_guard<std::mutex> lock(fg_all_armed_mutex);
    fg_all_armed=true;
    all_armed_cond_var.notify_one();
  }

}

static void on_all_stop(saftlib::Time time)
{
  std::cout << "All fgs stopped at " << tr_formatDate(time, PMODE_VERBOSE | (UTC?PMODE_UTC:PMODE_NONE)) << std::endl;
  {
    std::lock_guard<std::mutex> lock(fg_all_stopped_mutex);
    fg_all_stopped=true;
    all_stopped_cond_var.notify_one();
  }
}

// for thread safety tests
static void* startFg(void *arg) 
{
  std::cerr << ">>>thread started" << std::endl;
    
  std::ostringstream msg;
  msg << __FILE__<< "::" << __FUNCTION__ << ":"  << __LINE__ << " start loop";
  if (loglevel>1) {
    std::cout << msg.str() << std::endl; msg.str("");
  }
  while(true) {
    saftlib::wait_for_signal();
  }
  if (loglevel>1) std::cout << "startFg Loop ended" << std::endl;
  std::cerr << "<<<thread stopped" << std::endl;
  return NULL;
}



static void help(std::shared_ptr<SAFTd_Proxy> saftd)
{
  std::cerr << "Usage: fg-ctl [OPTION] < wavedata.txt\n";
  std::cerr << "\n";
  std::cerr << "  -d <device>   saftlib timing receiver device name\n";
  std::cerr << "  -f <fg-name>  name of function generator on device\n";
  std::cerr << "  -t <tag>      tag to use when triggering function generator\n";
  std::cerr << "  -e <id>       generate tag when timing event id is received\n";
  std::cerr << "  -g            generate tag immediately\n";
  std::cerr << "  -r            repeat the waveform over and over forever\n";
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

int main(int argc, char** argv)
{
  std::cout << "******************** saft-mfg-ctl start ******************" << std::endl;
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
    
    // Process command-line
    int opt, error = 0;
    while ((opt = getopt(argc, argv, "d:f:rgUht:e:")) != -1) {
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
        case 'U':
          UTC = true;
          break;
        case 't':
          tag = strtoul(optarg, 0, 0);
          break;
        case 'e':
          event = strtoul(optarg, 0, 0);
          eventSet = true;
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
    ParamSet params;
    int32_t a, la, b, lb, c, n, s, num;
    while((num = fscanf(stdin, "%d %d %d %d %d %d\n", &a, &la, &b, &lb, &c, &n)) == 6) {
      // turn off warning
      if (params.coeff_a.empty()) alarm(0);
      
      #define ACCU_OFFSET 40 /* don't ask */
      la += ACCU_OFFSET;
      lb += ACCU_OFFSET;
      s = 5; // 500kHz
      
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
    

    test_master_fg(scu, receiver, params, eventSet, event, repeat, generate, tag);
//    test_multiple_fgs(loop, scu, receiver, params, eventSet, event, repeat, generate, tag);

  } catch (const saftbus::Error& error) {
    std::cerr << "saftbus error: " << error.what() << std::endl;
  }

  std::cerr << "--------- main stopped" << std::endl;
  return 0;
}







void test_master_fg(std::shared_ptr<SCUbusActionSink_Proxy> scu, std::shared_ptr<TimingReceiver_Proxy> receiver, ParamSet params, bool eventSet, uint64_t event, bool repeat, bool generate, uint32_t tag)
{
    map<std::string, std::string> master_fgs = receiver->getInterfaces()["MasterFunctionGenerator"];            
    std::cerr << "Using Master Function Generator: " << master_fgs.begin()->second << std::endl;
    std::shared_ptr<MasterFunctionGenerator_Proxy> master_gen = MasterFunctionGenerator_Proxy::create(master_fgs.begin()->second);
   
    // Claim the function generator for ourselves
    master_gen->Own();
    // select all FGs    
    master_gen->ResetActiveFunctionGenerators(); 
		auto names = master_gen->ReadNames();
    
    for (auto name : names)
		{
			std::cout << name << "  ";
		} 		  
    std::cout << endl;

    // signals about the individual FGs for info
    master_gen->setGenerateIndividualSignals(true);
    master_gen->SigStarted.connect(sigc::ptr_fun(&on_master_started));
    master_gen->SigStopped.connect(sigc::ptr_fun(&on_master_stop));
    master_gen->Enabled.connect(sigc::ptr_fun(&on_master_enabled));
    master_gen->Armed.connect(sigc::ptr_fun(&on_master_armed));
    master_gen->Running.connect(sigc::ptr_fun(&on_master_running));
    master_gen->Refill.connect(sigc::ptr_fun(&on_master_refill));

    // master signals for control
    master_gen->SigAllStopped.connect(sigc::ptr_fun(&on_all_stop));
    master_gen->AllArmed.connect(&on_all_armed);

    // Stop whatever the function generator was doing
 		if (loglevel>0) std::cout << "Abort via Master" << std::endl;
    master_gen->Abort(false);
    
    // Wait until not Enabled - test polling as alternative to Abort(true)
    
    //std::shared_ptr<Glib::MainContext> context  = loop->get_context();

    vector<int> enabled_gens = master_gen->ReadEnabled();
    while (std::find(enabled_gens.begin(), enabled_gens.end(), 1) != enabled_gens.end()) 
    {      
      saftlib::wait_for_signal();
      enabled_gens = master_gen->ReadEnabled();
    }

    // threading test: start a background thread in which to perform wait_for_signal()
    // to test File Descriptor Async Methods
    
    pthread_t bg_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int ret=0;
    if ((ret = pthread_create(&bg_thread, &attr, &startFg, nullptr)) == 0) {
      if (loglevel>1) std::cout << "Created thread for receiving signals " << std::endl;      
    } 

  //for (int reps=0;reps<1;reps++)
  for (; names.size()>1; names.pop_back())
  {
    master_gen->SetActiveFunctionGenerators(names);

    // Clear any old waveform data
		if (loglevel>1) std::cout << "Flush via Master" << std::endl;
		master_gen->Flush();
 		if (loglevel>1) std::cout << "Flushed" << std::endl;
 		
 		master_gen->setStartTag(tag);
 		if (loglevel>0) std::cout << "Tag set" << std::endl;
    
    // Load up the parameters
		std::vector<std::vector<int16_t>> coeff_a;
		std::vector<std::vector<int16_t>> coeff_b;
		std::vector<std::vector<int32_t>> coeff_c;
    std::vector<std::vector<unsigned char>>step;
    std::vector<std::vector<unsigned char>>freq;
    std::vector<std::vector<unsigned char>>shift_a;
    std::vector<std::vector<unsigned char>>shift_b;
    // duplicate for each fg
		// empty vectors should not arm
		for (size_t i=0;i<names.size() && i < MAX_CHANNELS ;++i)
		{
     	coeff_a.push_back(params.coeff_a);
			coeff_b.push_back(params.coeff_b);
			coeff_c.push_back(params.coeff_c);
			step.push_back(params.step);
			freq.push_back(params.freq);
			shift_a.push_back(params.shift_a);
  		shift_b.push_back(params.shift_b);						
  		
		}

    // test arming methods
    const bool sync_arm=false;
    if(sync_arm)
    {
    std::cout << __FILE__ << __LINE__ << std::endl;
      if (loglevel>0) std::cout << "Loading and arming (sync) FG sets via Master " << coeff_a.size() << " * " << coeff_a[0].size() << std::endl;
      if (loglevel>1) timer_start();
      master_gen->AppendParameterSets(
      coeff_a,
      coeff_b,
      coeff_c,
      step,
      freq,
      shift_a,
      shift_b,
      true,  // arm?
      true); // arm ack?
      if (loglevel>1) timer_stop();    
      if (loglevel>0)
      {
        std::cout << "Loaded:      ";
        for (auto level : master_gen->ReadFillLevels())
        {
          std::cout << (level / 1000000.0) << "ms ";
        } 		   
        std::cout << endl;
      
        std::cout << "Armed" << std::endl;		
      }
    }
    else
    {
      if (loglevel>0) std::cout << "Loading and arming (async) FG sets via Master " << coeff_a.size() << " * " << coeff_a[0].size() << std::endl;
      {
        std::lock_guard<std::mutex> lock(fg_all_armed_mutex);
        fg_all_armed=false;
      }
      if (loglevel>1) timer_start();
      master_gen->AppendParameterSets(
      coeff_a,
      coeff_b,
      coeff_c,
      step,
      freq,
      shift_a,
      shift_b,
      false,  // arm?
      false); // arm ack?
      std::cout << "parameters sent" << std::endl;      
      master_gen->Arm();
      std::cout << "arm sent" << std::endl;      
      if (loglevel>1) timer_stop();    
      if (loglevel>0)
      {
        std::cout << "Loaded:      ";
        for (auto level : master_gen->ReadFillLevels())
        {
          std::cout << (level / 1000000.0) << "ms ";
        } 		   
        std::cout << endl;
      } 
      {
        std::unique_lock<std::mutex> lock(fg_all_armed_mutex);
        if (!fg_all_armed)
        {
          all_armed_cond_var.wait(lock, [](){ return fg_all_armed==true; });
        }
      }
    }
    
    if (loglevel>0)
    {
   		std::cout << "Fill Levels: ";
	  	for (auto level : master_gen->ReadFillLevels())
		  {
			  std::cout << (level / 1000000.0) << "ms ";
  		} 		   
      std::cout << endl;
    }
    

    {
      std::lock_guard<std::mutex> lock(fg_all_stopped_mutex);
      fg_all_stopped=false;
    }


		if (generate) {
	    if (loglevel>0) std::cout << "Generating StartTag" << std::endl;
	    scu->InjectTag(tag);
		}
    else if (eventSet) 
    {
      std::cout << "Waiting for event " << event << " to generate tag " << tag << std::endl;
      scu->NewCondition(true, event, ~0, 0, tag);
    }
    else
    {
      std::cout << "Waiting for tag " << tag << std::endl;
    }


    // Wait until master function generator reports all done
   
    {
      std::unique_lock<std::mutex> lock(fg_all_stopped_mutex);
      if (!fg_all_stopped)
      {
        all_stopped_cond_var.wait(lock, [](){ return fg_all_stopped==true; });
      }
    }
    if (loglevel>0)
    {
 		  std::cout << "Executed: ";
	  	for (auto count : master_gen->ReadExecutedParameterCounts())
  		{
		  	std::cout << count << " ";
	  	} 		   
   		std::cout << "Done" << std::endl;
    }
  } 
}

