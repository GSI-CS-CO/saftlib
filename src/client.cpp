#include <iostream>
#include <giomm.h>

#include "interfaces/Directory.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"

void on_action(guint64 id, guint64 param, guint64 time, bool late, bool conflict)
{
  std::cout << "Saw a timing event!" << std::endl;
}

using namespace saftlib;

int main(int, char**)
{
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  try {
    // Open the saftlib directory
    auto devices = Directory::create()->getDevices();
    
    // Grab a handle to the timing receiver
    auto receiver = TimingReceiver::create(devices["backplane"]);
    
    // Make a new software action sink
    auto sink = SoftwareActionSink::create(receiver->NewSoftwareActionSink(""));
    
    // Create an active(true) condition, watching events 5-200 delayed by 100 nanoseconds
    auto condition = SoftwareCondition::create(sink->NewCondition(true, 5, 200, 100, 0));
  
    // Call on_action whenever the condition is true
    condition->Action.connect(sigc::ptr_fun(&on_action));

    std::cout << "Waiting for timing events" << std::endl;
    loop->run();

  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}
