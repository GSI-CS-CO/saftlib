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
#include <pthread.h>

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

// Refill the function generator if the buffer fill gets low
static void on_refill(Glib::RefPtr<FunctionGenerator_Proxy> gen, const ParamSet& params)
{
  std::cerr << "refill in thread " << pthread_self() << std::endl;
  while (fill(gen, params)) { }
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

static void on_armed(Glib::RefPtr<SCUbusActionSink_Proxy> scu, guint64 tag)
{
  std::cout << "Generating StartTag" << std::endl;
  scu->InjectTag(tag);
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

static void help(Glib::RefPtr<SAFTd_Proxy> saftd)
{
  std::cerr << "Usage: saft-fg-ctl [OPTION] < wavedata.txt\n";
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


struct FgThreadData
{
  Glib::RefPtr<SCUbusActionSink_Proxy> scu;
  Glib::ustring fg;
  map<Glib::ustring, Glib::ustring> fgs;
  ParamSet params;
};

void *serve_fg(void *data) {
  FgThreadData *fgData = (FgThreadData*)data;
  Glib::RefPtr<SCUbusActionSink_Proxy> &scu = fgData->scu;
  Glib::ustring &fg = fgData->fg;
  map<Glib::ustring, Glib::ustring> &fgs = fgData->fgs;
  ParamSet &params = fgData->params;

  guint32 tag = 0xdeadbeef; // !!! fix me; use a safe default
  saftlib::SignalGroup fgSignalGroup;
  int error = 0;
  // Find the target FunctionGenerator (omit the check if it exists)
  // pass the saftlib::SignalGroup that is later used to do blocking wait 
  Glib::RefPtr<FunctionGenerator_Proxy> gen = FunctionGenerator_Proxy::create(fgs[fg], &fgSignalGroup);
  
  
  // Claim the function generator for ourselves,
  // stop whatever the function generator was doing,
  // wait until not Enabled,
  // and Clear any old waveform data
  gen->Own();
  gen->Abort();
  while (gen->getEnabled()) saftlib::wait_for_signal();
  gen->Flush();
  
  // Watch for events on the function generator
  gen->Started.connect(sigc::ptr_fun(&on_start));
  gen->Stopped.connect(sigc::ptr_fun(&on_stop));
  
  // Load up the parameters, possibly repeating until full
  while (fill(gen, params)) { }
  
  // Repeat the waveform forever!
  gen->Refill.connect(sigc::bind(sigc::ptr_fun(&on_refill), gen, params));
  
  // FYI, how much data is this?
  std::cout << "Loaded " << gen->ReadFillLevel() / 1000000.0 << "ms worth of waveform" << std::endl;
  
  // Trigger the function generator ourselves?
  gen->SigArmed.connect(sigc::bind(sigc::ptr_fun(&on_armed), scu, tag));
  
  // Ready to execute!
  gen->setStartTag(tag);
  gen->Arm();

  // the event loop
  while(true) {
    fgSignalGroup.wait_for_signal();
  }
  
  // Print summary
  std::cout << "Successful execution of " << gen->ReadExecutedParameterCount() << " polynomial tuples" << std::endl;

  return nullptr;
}

int main(int argc, char** argv)
{
  try {
    Gio::init();
    Glib::RefPtr<SAFTd_Proxy> saftd = SAFTd_Proxy::create();
    
    // Options
    Glib::ustring device;
    Glib::ustring fg;
    guint32 tag = 0xdeadbeef; // !!! fix me; use a safe default
    guint64 event = 0;
    bool eventSet = false;
    bool repeat = true;
    bool generate = true;
    
    // Process command-line
    int opt, error = 0;
    while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch (opt) {
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
    
    // Get a list of devices from the saftlib directory
    map<Glib::ustring, Glib::ustring> devices = saftd->getDevices();
    
    if (devices.empty()) {
      std::cerr << "No devices found" << std::endl;
      return 1;
    }
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices.begin()->second);
    
    // Confirm this device is an SCU
    map<Glib::ustring, Glib::ustring> scus = receiver->getInterfaces()["SCUbusActionSink"];
    if (scus.size() != 1) {
      std::cerr << "Device '" << receiver->getName() << "' is not an SCU" << std::endl;
      return 1;
    }
    Glib::RefPtr<SCUbusActionSink_Proxy> scu = SCUbusActionSink_Proxy::create(scus.begin()->second);
    
    // Get a list of function generators on the receiver
    map<Glib::ustring, Glib::ustring> fgs = receiver->getInterfaces()["FunctionGenerator"];
    
    FgThreadData fgData20;
    fgData20.scu = scu;
    fgData20.fg = "fg-2-0";
    fgData20.fgs = fgs;
    fgData20.params = params;

    FgThreadData fgData21 = fgData20;
    fgData21.fg = "fg-2-1";

    pthread_t p1, p2;

    std::cerr << "main thread " << pthread_self() << std::endl;
    pthread_create(&p1, nullptr, serve_fg, &fgData20);
    pthread_create(&p2, nullptr, serve_fg, &fgData21);

    pthread_join(p1,nullptr);
    pthread_join(p2,nullptr);
    //serve_fg(&fgData20);

  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}


