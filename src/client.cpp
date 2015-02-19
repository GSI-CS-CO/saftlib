#include <iostream>
#include <giomm.h>
#include "ECA.h"

int main(int, char**)
{
  Gio::init();
  
  try {
  
    Glib::RefPtr<saftlib::ECA_Proxy> proxy = saftlib::ECA_Proxy::create_for_bus_sync(
      Gio::DBus::BUS_TYPE_SESSION, "de.gsi.saftlib", "/de/gsi/saftlib/ECA");
      
    proxy->Poke();
    
    std::map<Glib::ustring, std::vector< gint32 > > demo;
    demo["hello"].push_back(5);
    demo["hello"].push_back(8);
    demo["hello"].push_back(9);
    demo["cat"].push_back(42);
    
    gint16 result;
    proxy->Listen(44, 0, 2, demo, result);
    std::cout << "Result: " << result << std::endl;
    
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to invoke method: " << error.what() << std::endl;
  }

  return 0;
}
