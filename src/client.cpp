#define ETHERBONE_THROWS 1

#include <iostream>
#include <giomm.h>
#include "interfaces/Directory.h"
#include "interfaces/TLU.h"
#include "interfaces/ECA.h"
#include "interfaces/ECA_Condition.h"

Glib::RefPtr<saftlib::TLU_Proxy> channel;
void on_edge(guint64 time) {
  guint64 now = channel->CurrentTime();
  std::cout << "Pulse detected: " << time << " @ " << now << " = " << (now-time) << std::endl;
}

void on_action(guint64 id, guint64 param, guint64 time, guint32 tef, bool late, bool conflict)
{
  std::cout << "Saw a timing event!" << std::endl;
}

int main(int, char**)
{
  Gio::init();
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  
  try {
    // Open the saftlib directory
    std::map< Glib::ustring, std::vector< Glib::ustring > > devices = 
      saftlib::Directory_Proxy::create()->getDevices();
    
    // Play with the TLU
    channel = saftlib::TLU_Proxy::create(devices["TLU"][0]);
    
    // Was it already active?
    std::cout << "Channel was: " << (channel->getEnabled()?"active":"inactive") << std::endl;
    
    // Listen for edges
    channel->Edge.connect(sigc::ptr_fun(&on_edge));
    channel->setLatchEdge(false);
    channel->setStableTime(16);
    channel->setEnabled(true);
    
    // Play with the ECA
    Glib::RefPtr<saftlib::ECA_Proxy> eca = 
      saftlib::ECA_Proxy::create(devices["ECA"][0]);
    
    // Create a condition, watching events 5-200 delayed by +100*8 nanoseconds
    Glib::RefPtr<saftlib::ECA_Condition_Proxy> condition = 
      saftlib::ECA_Condition_Proxy::create(
        eca->NewCondition(5, 200, 100));
  
    // Watch the condition
    condition->Action.connect(sigc::ptr_fun(&on_action));

    std::cout << "Waiting for TLU edges or ECA events" << std::endl;
    loop->run();
    
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}
