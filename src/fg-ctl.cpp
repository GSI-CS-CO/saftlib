#include <iostream>
#include <giomm.h>
#include <stdio.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SCUbusActionSink.h"
#include "interfaces/SCUbusCondition.h"
#include "interfaces/FunctionGenerator.h" 

using namespace saftlib;
using namespace std;

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

// Hand off the entire datafile to SAFTd
void fill(Glib::RefPtr<FunctionGenerator_Proxy> gen, const ParamSet& params)
{
  gen->AppendParameterSet(
    params.coeff_a,
    params.coeff_b,
    params.coeff_c,
    params.step,
    params.freq,
    params.shift_a,
    params.shift_b);
}

// Refill the function generator if the buffer fill gets low
void on_lowFillLevel(bool aboveSafeFillLevel, Glib::RefPtr<FunctionGenerator_Proxy> gen, const ParamSet& params)
{
  if (aboveSafeFillLevel) return;
  
  do fill(gen, params);
  while (!gen->getAboveSafeFillLevel());
}

// Pretty print timestamp
const char *format_time(guint64 time)
{
  static char full[80];
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09d", date, ns);
  return full;
}

// Report when the function generator starts
void on_start(guint64 time)
{
  std::cout << "Function generator started at " << format_time(time) << std::endl;
}

// Report when the function generator stops
void on_stop(guint64 time, bool hardwareMacroUnderflow, bool microControllerUnderflow, Glib::RefPtr<Glib::MainLoop> loop)
{
  std::cout << "Function generator stopped at " << format_time(time) << std::endl;
  // was there an error?
  if (hardwareMacroUnderflow)
    std::cerr << "Fatal error: hardwareMacroUnderflow!" << std::endl;
  if (microControllerUnderflow)
    std::cerr << "Fatal error: microControllerUnderflow!" << std::endl;
  // terminate the main event loop
  loop->quit();
}

int main(int argc, char** argv)
{
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  // Options
  Glib::ustring device;
  Glib::ustring fg;
  guint32 tag = 0xdeadbeef; // !!! fix me; use a safe default
  guint64 event;
  bool eventSet = false;
  bool repeat = false;
  bool generate = false;
  
  // Process command-line
  int opt, error = 0;
  while ((opt = getopt(argc, argv, "d:f:rgt:e:")) != -1) {
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
      case ':':
      case '?':
        error = 1;
        break;
      default:
        std::cerr << argv[0] << ": bad getopt result" << std::endl;
        error = 1;
    }
  }
  
  if (error) return 1;
  
  // Read the data file from stdin ... maybe come up with a better format in the future !!!
  ParamSet params;
  gint32 a, la, b, lb, c, n, s;
  while(fscanf(stdin, "%d %d %d %d %d %d\n", &a, &la, &b, &lb, &c, &n) == 6) {
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
  
  if (params.shift_b.empty()) {
    std::cerr << "Provided data file was empty" << std::endl;
    return 1;
  }
  
  try {
    // Get a list of devices from the saftlib directory
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    
    // Find the requested device
    Glib::RefPtr<TimingReceiver_Proxy> receiver;
    if (device.empty()) {
      if (devices.size() != 1) {
        std::cerr << "Not exactly one device; specify a device with '-d <device>'" << std::endl;
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
      std::cerr << "Device '" << device << "' is not an SCU" << std::endl;
      return 1;
    }
    Glib::RefPtr<SCUbusActionSink_Proxy> scu = SCUbusActionSink_Proxy::create(scus.begin()->second);
    
    // Get a list of function generators on the receiver
    map<Glib::ustring, Glib::ustring> fgs = receiver->getInterfaces()["FunctionGenerator"];
    
    // Find the target FunctionGenerator
    Glib::RefPtr<FunctionGenerator_Proxy> gen;
    if (fg.empty()) {
      if (fgs.size() != 1) {
        std::cerr << "Not exactly one function generator; specify a device with '-f <function-generator>'" << std::endl;
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
      for (map<Glib::ustring, Glib::ustring>::iterator i = fgs.begin(); i != fgs.end(); ++i)
        std::cerr << "  " << i->first << std::endl;
      return 1;
    }
    
    // Ok! Find all the devices is now out of the way. Let's get some work done.
    
    // Claim the function generator for ourselves
    gen->Own();
    
    // Make sure it will not trigger behind our back during configuration
    gen->setEnabled(false);
    
    // Better not be already running!
    if (gen->getRunning()) {
      std::cerr << "Function generator is already running!" << std::endl;
      return 1;
    }
    
    // Clear any old waveform data
    gen->Flush();
    
    // Watch for events on the function generator
    gen->Started.connect(sigc::ptr_fun(&on_start));
    gen->Stopped.connect(sigc::bind(sigc::ptr_fun(&on_stop), loop));
    
    // Load up the parameters
    fill(gen, params);
    
    // Repeat the waveform forever?
    if (repeat) {
      // repeat waveform till safely full
      while (!gen->getAboveSafeFillLevel()) fill(gen, params); 
      // listen to refill indicator
      gen->AboveSafeFillLevel.connect(sigc::bind(sigc::ptr_fun(&on_lowFillLevel), gen, params));
    }
    
    // FYI, how much data is this?
    std::cout << "Loaded " << gen->getFillLevel() / 1000000.0 << "ms worth of waveform" << std::endl;
    
    // Ready to execute!
    gen->setStartTag(tag);
    gen->setEnabled(true);
    
    if (generate) { // Trigger the function generator ourselves?
      scu->InjectTag(tag);
    } else if (eventSet) { // Watch for a timing event? => generate tag on event
      scu->NewCondition(true, event, ~0, 0, 0, tag);
    }
    
    // Wait until on_stop is called / function generator is done
    // ... if we don't care to keep repeating the waveform, we could quit immediately;
    //     SAFTd has been properly configured to run autonomously at this point.
    // ... of course, then the user doesn't get the final error status message either
    loop->run();
    
    // Print summary
    std::cout << "Successful execution of " << gen->getExecutedParameterCount() << " polynomial tuples" << std::endl;

  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}
