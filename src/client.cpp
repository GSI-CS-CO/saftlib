#include <iostream>
#include <giomm.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"

static guint64 mask(int i) {
  return i ? (((guint64)-1) << (64-i)) : 0;
}

void on_action(guint64 id, guint64 param, guint64 time, guint64 overtime, bool late, bool delayed, bool conflict)
{
  std::cout << "Saw a timing event!" << std::endl;
  std::cout << "  " << id << " " << time << " " << conflict << std::endl;
}

void on_late(guint64 count, guint64 event, guint64 param, guint64 time, guint64 overtime)
{
  // Report this to the operator
  std::cerr << "FATAL ERROR: late action!" << std::endl;
}

void on_overflow(guint64 count)
{
  std::cerr << "FATAL ERROR: lost action!" << std::endl;
}

void on_conflict(guint64 count, 
                 guint64 event1, guint64 param1, guint64 time1, 
                 guint64 event2, guint64 param2, guint64 time2)
{
  std::cerr << "FATAL ERROR: actions potentially misordered!" << std::endl;
}

void detect_danger(guint32 capacity, guint32 mostFull)
{
  // Is the hardware more than 50% full? getting dangerous
  if (mostFull > capacity/2) {
    std::cerr << "WARNING: hardware is more than 50%% full. An Overflow may be imminent." << std::endl;
  }
}

using namespace saftlib;
using namespace std;

int main(int, char**)
{
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  try {
    // Get a list of devices from the saftlib directory
    // The dbus type 'a{ss}' means: map<string, string>
    map<Glib::ustring, Glib::ustring> devices = SAFTd_Proxy::create()->getDevices();
    
    // Grab a handle to the timing receiver attached to an SCU
    // 
    // The '::create' syntax creates a proxy object that provides the local
    // program with access to the remote object stored inside saftd. 
    // Returned is a smart pointer (with copy constructor).  Once the number
    // of references reaches zero, the proxy object is automatically freed.
    Glib::RefPtr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices["baseboard"]);
    
    // Direct saftd to create a new SoftwareActionSink for this program.
    // The name is "", so one is chosen automatically that does not conflict.
    // The returned object (a SoftwareActionSink) implements these interfaces:
    //   iOwned, iActionSink, and iSoftwareActionSink
    Glib::RefPtr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
    
    // No one should care if software actions are delayed (false is default)
    sink->setGenerateDelayed(false);
    
    // Attach handlers watching for all failure conditions
    sink->Late.connect(sigc::ptr_fun(&on_late));
    sink->Overflow.connect(sigc::ptr_fun(&on_overflow));
    sink->Conflict.connect(sigc::ptr_fun(&on_conflict));
    
    // Our SoftwareActionSink was freshly created, so these are already 0
    // We set them to 0 anyway, for illustration
    sink->setLateCount(0);
    // sink->setOverflowCount(0);
    sink->setConflictCount(0);
  
    // Read the Capacity property of the ActionSink
    guint32 capacity = sink->getCapacity();
    
    // Clear the MostFull property to zero
    // Unlike {Late,Overflow,Conflict}Count, the MostFull count may be non-zero
    // since it is shared between all users of SoftwareActionSinks.
    sink->setMostFull(0);
    
    // Watch changes to the MostFull property
    // Here, we provide the first function parameter via 'bind'.
    sink->MostFull.connect(sigc::bind(sigc::ptr_fun(&detect_danger), capacity));
    
    // Create an active(true) condition, watching events 64-127 delayed by 100 nanoseconds
    // When NewCondition is run on a SoftwareActionSink, result is a SoftwareCondition.
    // SoftwareConditions implement iOwned, iCondition, iSoftwareCondition.
    Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(true, 64, mask(58), 100, 0));
    
    // Call on_action whenever the condition above matches incoming events.
    condition->Action.connect(sigc::ptr_fun(&on_action));

    // Run the Glib event loop
    // Inside callbacks you can still run all the methods like we did above
    std::cout << "Waiting for timing events" << std::endl;
    loop->run();

  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }
  
  // Upon program exit, all Owned objects are automatically Destroyed.
  // This means the SoftwareActionSink and SoftwareCondition

  return 0;
}
