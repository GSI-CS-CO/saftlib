#include <iostream>
#include <giomm.h>

#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"

static guint64 mask(int i) {
  return i ? (((guint64)-1) << (64-i)) : 0;
}

void on_action(guint64 id, guint64 param, guint64 time, guint64 overtime, bool late, bool delayed, bool conflict)
{
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40], full[80];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09d", date, ns);
  
  std::cout << std::hex << "Time: " << full << " ID: 0x" << id << " Param: 0x" << param << " " << ((conflict||late||delayed)?"!":"") << std::endl;
}

using namespace saftlib;
using namespace std;

int main(int, char**)
{
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  try {
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices.begin()->second);
    Glib::RefPtr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
    Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(true, 0, mask(0), 0, 0));
    condition->Action.connect(sigc::ptr_fun(&on_action));
    loop->run();

  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  return 0;
}
