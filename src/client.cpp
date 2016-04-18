/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include <stdio.h>
#include <iostream>
#include <giomm.h>

#include "interfaces/SAFTd.h"
#include "interfaces/TimingReceiver.h"
#include "interfaces/SoftwareActionSink.h"
#include "interfaces/SoftwareCondition.h"

static guint64 mask(int i) {
  return i ? (((guint64)-1) << (64-i)) : 0;
}

// format date string 
static std::string formatDate(guint64 time)
{
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  static char full[80];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns);

  return full;
} // format date

static void onLocked(bool locked)
{
  if (locked) {
    std::cout << "WR Locked!" << std::endl;
  } else {
    std::cout << "WR Lost Lock!" << std::endl;
  }
}

static void onMostFull(guint16 full, guint16 capacity)
{
  std::cout << "MostFull: " << full << "/" << capacity << std::endl;
}

static void onOverflowCount(guint64 count)
{
  std::cout << "OverflowCount: " << count << std::endl;
}

static void onActionCount(guint64 count)
{
  std::cout << "ActionCount: " << count << std::endl;
}

static void onLateCount(guint64 count)
{
  std::cout << "LateCount: " << count << std::endl;
}

static void onLate(guint64 count, guint64 event, guint64 param, guint64 deadline, guint64 executed)
{
  std::cout
   << "Late #" << count << ": 0x" << std::hex << event << " " << param << " at " 
   << formatDate(executed) << " (should be " << formatDate(deadline) << ")" << std::endl;
}

static void onEarlyCount(guint64 count)
{
  std::cout << "EarlyCount: " << count << std::endl;
}

static void onEarly(guint64 count, guint64 event, guint64 param, guint64 deadline, guint64 executed)
{
  std::cout
   << "Early #" << count << ": 0x" << std::hex << event << " " << param << " at " 
   << formatDate(executed) << " (should be " << formatDate(deadline) << ")" << std::endl;
}

static void onConflictCount(guint64 count)
{
  std::cout << "ConflictCount: " << count << std::endl;
}

static void onConflict(guint64 count, guint64 event, guint64 param, guint64 deadline, guint64 executed)
{
  std::cout
   << "Conflict #" << count << ": 0x" << std::hex << event << " " << param << " at " 
   << formatDate(executed) << " (should be " << formatDate(deadline) << ")" << std::endl;
}

static void onDelayedCount(guint64 count)
{
  std::cout << "DelayedCount: " << count << std::endl;
}

static void onDelayed(guint64 count, guint64 event, guint64 param, guint64 deadline, guint64 executed)
{
  std::cout
   << "Delayed #" << count << ": 0x" << std::hex << event << " " << param << " at " 
   << formatDate(executed) << " (should be " << formatDate(deadline) << ")" << std::endl;
}

static void onAction(guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags, int rule)
{
  std::cout
    << "Condition #" << rule << ": 0x" << std::hex << event << " " << param << " at " 
    << formatDate(executed) << " (should be " << formatDate(deadline) << ") " << flags << std::endl;
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
    
    // Monitor the WR lock status
    receiver->Locked.connect(sigc::ptr_fun(&onLocked));

    // Direct saftd to create a new SoftwareActionSink for this program.
    // The name is "", so one is chosen automatically that does not conflict.
    // The returned object (a SoftwareActionSink) implements these interfaces:
    //   iOwned, iActionSink, and iSoftwareActionSink
    Glib::RefPtr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink(""));
    
    // Read the Capacity property of the ActionSink
    guint16 capacity = sink->getCapacity();
    
    // Watch changes to the MostFull property
    // Here, we provide the first function parameter via 'bind'.
    sink->MostFull.connect(sigc::bind(sigc::ptr_fun(&onMostFull), capacity));
    
    // Clear the MostFull property to zero
    // Unlike {Late,Early,Overflow,Conflict}Count, the MostFull count may be
    // non-zero since it is shared between all users of SoftwareActionSinks.
    sink->setMostFull(0);
    
    // Attach handlers watching for all failure conditions
    sink->OverflowCount.connect(sigc::ptr_fun(&onOverflowCount));
    sink->ActionCount.connect(sigc::ptr_fun(&onActionCount));
    sink->LateCount.connect(sigc::ptr_fun(&onLateCount));
    sink->Late.connect(sigc::ptr_fun(&onLate));
    sink->EarlyCount.connect(sigc::ptr_fun(&onEarlyCount));
    sink->Early.connect(sigc::ptr_fun(&onEarly));
    sink->ConflictCount.connect(sigc::ptr_fun(&onConflictCount));
    sink->Conflict.connect(sigc::ptr_fun(&onConflict));
    sink->DelayedCount.connect(sigc::ptr_fun(&onDelayedCount));
    sink->Delayed.connect(sigc::ptr_fun(&onDelayed));
    
    // Create an active(true) condition, watching events 64-127 delayed by 100 nanoseconds
    // When NewCondition is run on a SoftwareActionSink, result is a SoftwareCondition.
    // SoftwareConditions implement iOwned, iCondition, iSoftwareCondition.
    Glib::RefPtr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(true, 64, mask(58), 0));
    
    // Call on_action whenever the condition above matches incoming events.
    condition->Action.connect(sigc::bind(sigc::ptr_fun(&onAction), 0));
    
    // Generate an event that matches
    receiver->InjectEvent(88, 0xdeadbeef, receiver->ReadCurrentTime() + 1000000000L);
    
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
