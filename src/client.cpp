#include <iostream>
#include <giomm.h>
#include "../interfaces/ECA.h"

// The main loop.
Glib::RefPtr<Glib::MainLoop> loop;
Glib::RefPtr<saftlib::ECA_Proxy> proxy;

void on_prop(const Glib::ustring& val) {
  std::cout << "Saw name change: " << val << std::endl;
  std::cout << "name: " << proxy->getName() << std::endl;
}

void on_signal(const Glib::ustring& reason) {
  std::cout << "They cry? " << reason << std::endl;
}

int main(int, char**)
{
  Gio::init();

  try {
  
    loop = Glib::MainLoop::create();
    proxy = saftlib::ECA_Proxy::create_for_bus_sync(
      Gio::DBus::BUS_TYPE_SESSION, "de.gsi.saftlib", "/de/gsi/saftlib/ECA");
    
    // Listen for signals
    proxy->Name.connect(sigc::ptr_fun(&on_prop));
    proxy->Cry.connect(sigc::ptr_fun(&on_signal));

    std::cout << "name: " << proxy->getName() << std::endl;
    proxy->Poke();
      
    loop->run();
    
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}
