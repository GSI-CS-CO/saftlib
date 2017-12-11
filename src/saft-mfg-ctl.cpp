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
#include <giomm.h>
#include <stdio.h>
#include <unistd.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SCUbusActionSink.h"
#include "interfaces/SCUbusCondition.h"
#include "interfaces/FunctionGenerator.h" 
#include "interfaces/MasterFunctionGenerator.h" 

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
  std::vector< gint16 > coeff_a;
  std::vector< gint16 > coeff_b;
  std::vector< gint32 > coeff_c;
  std::vector< unsigned char > step;
  std::vector< unsigned char > freq;
  std::vector< unsigned char > shift_a;
  std::vector< unsigned char > shift_b;
};

  
void test_master_fg(Glib::RefPtr<Glib::MainLoop> loop,Glib::RefPtr<SCUbusActionSink_Proxy> scu, Glib::RefPtr<TimingReceiver_Proxy> receiver, ParamSet params, bool eventSet, guint64 event, bool repeat, bool generate, guint32 tag);
void test_multiple_fgs(Glib::RefPtr<Glib::MainLoop> loop,Glib::RefPtr<SCUbusActionSink_Proxy> scu, Glib::RefPtr<TimingReceiver_Proxy> receiver, ParamSet params, bool eventSet, guint64 event, bool repeat, bool generate, guint32 tag);


// Hand off the entire datafile to SAFTd
static bool fill(Glib::RefPtr<FunctionGenerator_Proxy> gen, const ParamSet& params)
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

// Pretty print timestamp
static const char *format_time(guint64 time)
{
  static char full[80];
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns);
  return full;
}

static void on_armed(bool armed, Glib::RefPtr<SCUbusActionSink_Proxy> scu, guint64 tag)
{
  if (armed) {
  
    std::cout << "Generating StartTag in 2 seconds" << std::endl;
    sleep(2);
    scu->InjectTag(tag);
    std::cout << "Generating StartTag" << std::endl;
  }
}


// Report when the function generator starts
static void on_start(guint64 time)
{
  std::cout << "Function generator started at " << format_time(time) << std::endl;
}

// Report when the function generator stops
static void on_stop(guint64 time, bool abort, bool hardwareMacroUnderflow, bool microControllerUnderflow)
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

// Report on the individual function generators
static void on_master_started(Glib::ustring fg_name, guint64 time)
{
  if (loglevel>0) std::cout << fg_name << " started at " << format_time(time) << std::endl;
}
static void on_master_armed(Glib::ustring fg_name, bool armed)
{
  if (loglevel>0) std::cout << fg_name << (armed ? " armed" : " disarmed")  << std::endl;
}
static void on_master_enabled(Glib::ustring fg_name, bool enabled)
{
  if (loglevel>0) std::cout << fg_name << (enabled ? " enabled" : " disabled")  << std::endl;
}
static void on_master_running(Glib::ustring fg_name, bool running)
{
  if (loglevel>0) std::cout << fg_name << (running ? " running" : " not running") << std::endl;
}
static void on_master_refill(Glib::ustring fg_name)
{
  if (loglevel>0) std::cout << fg_name << " refill request" << std::endl;
}
static void on_master_stop(Glib::ustring fg_name, guint64 time, bool abort, bool hardwareMacroUnderflow, bool microControllerUnderflow)
{
  if (loglevel>0) std::cout << fg_name << " stopped at " << format_time(time) << std::endl;
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

static void on_all_stop(guint64 time, Glib::RefPtr<Glib::MainLoop> loop)
{
  std::cout << "All fgs stopped at " << format_time(time) << std::endl;
  {
    std::lock_guard<std::mutex> lock(fg_all_stopped_mutex);
    fg_all_stopped=true;
    all_stopped_cond_var.notify_one();
  }
}


// When the function generator becomes disabled, stop the loop
static void on_enabled(bool value, Glib::RefPtr<Glib::MainLoop> loop)
{
  if (value) return;
  // terminate the main event loop
  loop->quit();
}

// for thread safety tests
static void* startFg(void *arg) 
{
  Glib::RefPtr<Glib::MainLoop> *tmp = (Glib::RefPtr<Glib::MainLoop> *)arg;
  Glib::RefPtr<Glib::MainLoop> loop = *tmp;
    
  //std::cout << "startFg Loop created" << std::endl;
  if (!loop->is_running()) {
    std::ostringstream msg;
    msg << __FILE__<< "::" << __FUNCTION__ << ":"  << __LINE__ << " start loop";
    if (loglevel>1) std::cout << msg.str() << std::endl; msg.str("");
    loop->run();
  }
  if (loglevel>1) std::cout << "startFg Loop ended" << std::endl;
  return NULL;
}



static void help(Glib::RefPtr<SAFTd_Proxy> saftd)
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
  try {
    Gio::init();
    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    Glib::RefPtr<SAFTd_Proxy> saftd = SAFTd_Proxy::create();
    
    // Options
    Glib::ustring device;
    Glib::ustring fg;
    guint32 tag = 0xdeadbeef; // !!! fix me; use a safe default
    guint64 event = 0;
    bool eventSet = false;
    bool repeat = false;
    bool generate = false;
    
    // Process command-line
    int opt, error = 0;
    while ((opt = getopt(argc, argv, "d:f:rght:e:")) != -1) {
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
    gint32 a, la, b, lb, c, n, s, num;
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
    map<Glib::ustring, Glib::ustring> devices = saftd->getDevices();
    
    // Find the requested device
    Glib::RefPtr<TimingReceiver_Proxy> receiver;
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
      for (map<Glib::ustring, Glib::ustring>::iterator i = devices.begin(); i != devices.end(); ++i)
        std::cerr << "  " << i->first << std::endl;
      return 1;
    }
    
    // Confirm this device is an SCU
    map<Glib::ustring, Glib::ustring> scus = receiver->getInterfaces()["SCUbusActionSink"];
    if (scus.size() != 1) {
      std::cerr << "Device '" << receiver->getName() << "' is not an SCU" << std::endl;
      return 1;
    }
    Glib::RefPtr<SCUbusActionSink_Proxy> scu = SCUbusActionSink_Proxy::create(scus.begin()->second);
    

    test_master_fg(loop, scu, receiver, params, eventSet, event, repeat, generate, tag);
//    test_multiple_fgs(loop, scu, receiver, params, eventSet, event, repeat, generate, tag);

  } catch (const Glib::Error& error) {
    std::cerr << "Glib error: " << error.what() << std::endl;
  }
  
  return 0;
}







void test_master_fg(Glib::RefPtr<Glib::MainLoop> loop,Glib::RefPtr<SCUbusActionSink_Proxy> scu, Glib::RefPtr<TimingReceiver_Proxy> receiver, ParamSet params, bool eventSet, guint64 event, bool repeat, bool generate, guint32 tag)
{
    map<Glib::ustring, Glib::ustring> master_fgs = receiver->getInterfaces()["MasterFunctionGenerator"];            
    std::cerr << "Using Master Function Generator: " << master_fgs.begin()->second << std::endl;
    Glib::RefPtr<MasterFunctionGenerator_Proxy> master_gen = MasterFunctionGenerator_Proxy::create(master_fgs.begin()->second);
   
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
    master_gen->Started.connect(sigc::ptr_fun(&on_master_started));
    master_gen->Stopped.connect(sigc::ptr_fun(&on_master_stop));
    master_gen->Enabled.connect(sigc::ptr_fun(&on_master_enabled));
    master_gen->Armed.connect(sigc::ptr_fun(&on_master_armed));
    master_gen->Running.connect(sigc::ptr_fun(&on_master_running));
    master_gen->Refill.connect(sigc::ptr_fun(&on_master_refill));

    // master signals for control
    master_gen->AllStopped.connect(sigc::bind(sigc::ptr_fun(&on_all_stop), loop));
    master_gen->AllArmed.connect(sigc::ptr_fun(&on_all_armed));

    // Stop whatever the function generator was doing
 		if (loglevel>0) std::cout << "Abort via Master" << std::endl;
    master_gen->Abort(false);
    
    // Wait until not Enabled - test polling as alternative to Abort(true)
    
    Glib::RefPtr<Glib::MainContext> context  = loop->get_context();

    vector<int> enabled_gens = master_gen->ReadEnabled();
    while (std::find(enabled_gens.begin(), enabled_gens.end(), 1) != enabled_gens.end()) 
    {      
      context->iteration(false);
      enabled_gens = master_gen->ReadEnabled();
    }

    // threading test: start a background thread in which to perform mainloop->run
    // to test File Descriptor Async Methods
    
    pthread_t bg_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int ret=0;
    if ((ret = pthread_create(&bg_thread, &attr, &startFg, &loop)) == 0) {
      if (loglevel>1) std::cout << "Created thread for mainloop " << std::endl;      
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
		std::vector<std::vector<gint16>> coeff_a;
		std::vector<std::vector<gint16>> coeff_b;
		std::vector<std::vector<gint32>> coeff_c;
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

// test multiple fgs triggering on the same start tag
// use the individual FunctionGenerator dbus interface
// use the last fg's signals
void test_multiple_fgs(Glib::RefPtr<Glib::MainLoop> loop,Glib::RefPtr<SCUbusActionSink_Proxy> scu, Glib::RefPtr<TimingReceiver_Proxy> receiver, ParamSet params, bool eventSet, guint64 event, bool repeat, bool generate, guint32 tag)
{
// Get a list of function generators on the receiver
    map<Glib::ustring, Glib::ustring> fgs = receiver->getInterfaces()["FunctionGenerator"];

  std::vector<std::string> names = {"fg-2-0","fg-2-1"};
//    std::vector<std::string> names = {"fg-3-0","fg-3-1","fg-4-0","fg-4-1","fg-5-0","fg-5-1","fg-6-0","fg-6-1","fg-8-0","fg-8-1","fg-9-0","fg-9-1"};
//		std::vector<std::string> names = {"fg-3-0","fg-3-1","fg-4-0","fg-4-1","fg-5-0","fg-5-1","fg-6-0","fg-6-1"};
//		std::vector<std::string> names = {"fg-3-0","fg-3-1","fg-4-0","fg-4-1","fg-5-0","fg-5-1","fg-6-0"};
		std::vector< Glib::RefPtr<FunctionGenerator_Proxy> > gens;
		for (auto name : names)
		{	
			gens.push_back(FunctionGenerator_Proxy::create(fgs[name]));
		}
    
   // Ok! Find all the devices is now out of the way. Let's get some work done.
    std::cout << "Using Proxies for ";  
    for (auto name : names)
      std::cout << name << ",";
    std::cout << std::endl;

    // Claim the function generator for ourselves
  for (auto gen : gens)
    gen->Own();
    
    // Stop whatever the function generator was doing
  for (auto gen : gens)    
    gen->Abort();
    
    // Wait until not Enabled
    gens.back()->Enabled.connect(sigc::bind(sigc::ptr_fun(&on_enabled), loop));
    if (gens.back()->getEnabled()) loop->run();
    
    // Clear any old waveform data
  for (auto gen : gens)    
    gen->Flush();
    
    // Watch for events on the function generator
    gens.back()->Started.connect(sigc::ptr_fun(&on_start));
    gens.back()->Stopped.connect(sigc::ptr_fun(&on_stop));
    
    // Load up the parameters, possibly repeating until full
  for (auto gen : gens)
  {    
    while (fill(gen, params) && repeat) { }
  }  
    // Repeat the waveform forever?
//    if (repeat) {
      // listen to refill indicator
//      gen->Refill.connect(sigc::bind(sigc::ptr_fun(&on_refill), gen, params));
//    }
    
    // FYI, how much data is this?
 for (auto gen : gens)
    std::cout << "Loaded " << gen->ReadFillLevel() / 1000000.0 << "ms worth of waveform" << std::endl;
    
    // Watch for a timing event? => generate tag on event
    if (eventSet) scu->NewCondition(true, event, ~0, 0, tag);

    // Trigger the function generator ourselves?
    if (generate)
    { 
    	gens.back()->Armed.connect(sigc::bind(sigc::ptr_fun(&on_armed), scu, tag));
    }
    
    
    // Ready to execute!
			for (auto gen : gens)
			{    
				gen->setStartTag(tag);
				gen->Arm();
	  		std::cout << "Arming " << std::endl; 
			}
			
  		std::cout << "All Armed" << std::endl; 
  		
			if (generate)
			{
				// Wait until not enabled / function generator is done
				// ... if we don't care to keep repeating the waveform, we could quit immediately;
				//     SAFTd has been properly configured to run autonomously at this point.
				// ... of course, then the user doesn't get the final error status message either
				loop->run();
		    sleep(1);
        loop->get_context()->iteration(false);
        loop->get_context()->iteration(false);
        loop->get_context()->iteration(false);
				// Print summary
				for (auto gen : gens)
					std::cout << "Successful execution of " << gen->ReadExecutedParameterCount() << " polynomial tuples" << std::endl;
			}
			else
			{
		  		std::cout << "Not generating Starttag, waiting forever" << std::endl; 
					loop->run();
			}
}

