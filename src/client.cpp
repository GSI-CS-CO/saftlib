#define ETHERBONE_THROWS 1

#include <iostream>
#include <giomm.h>
#include "interfaces/Directory.h"
#include "interfaces/TLU.h"
#include "interfaces/ECA.h"
#include "interfaces/ECA_Condition.h"

// something like this will move into final library
#define create_(x) create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION, "de.gsi.saftlib", x)

Glib::RefPtr<saftlib::TLU_Proxy> channel;
void on_edge(const guint64& time) {
  guint64 now;
  channel->CurrentTime(now);
  std::cout << "Pulse detected: " << time << " @ " << now << " = " << (now-time) << std::endl;
}

void on_action(
  const guint64& id, const guint64& param, const guint64& time, 
  const guint32& tef, const bool& late, const bool& conflict)
{
  std::cout << "Saw a timing event!" << std::endl;
}

int main(int, char**)
{
  Gio::init();
  auto loop = Glib::MainLoop::create();
  
  try {
    // Open the saftlib directory
    auto directory = saftlib::Directory_Proxy::create_("/de/gsi/saftlib/Directory");
    
    // Play with the TLU
    channel = saftlib::TLU_Proxy::create_(directory->getDevices()["TLU"][0]);
    
    // Was it already active?
    std::cout << "Channel was: " << (channel->getEnabled()?"active":"inactive") << std::endl;
    
    // Listen for edges
    channel->Edge.connect(sigc::ptr_fun(&on_edge));
    channel->setLatchEdge(false);
    channel->setStableTime(16);
    channel->setEnabled(true);
    
    // Play with the ECA
    auto eca = saftlib::ECA_Proxy::create_(directory->getDevices()["ECA"][0]);
    
    // Create a condition, watching events 5-100 delayed by +100*8 nanoseconds
    Glib::ustring path;
    eca->NewCondition(5, 100, 100, path);
    auto condition = saftlib::ECA_Condition_Proxy::create_(path);
  
    // Watch the condition
    condition->Action.connect(sigc::ptr_fun(&on_action));

    std::cout << "Waiting for TLU edges or ECA events" << std::endl;
    loop->run();
    
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}
