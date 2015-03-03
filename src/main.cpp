#define ETHERBONE_THROWS 1

#include <iostream>
#include <giomm.h>

#include "ObjectRegistry.h"
#include "Driver.h"
#include "eb-source.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  try {
    saftlib::ObjectRegistry::register_all(connection);
  } catch(const Glib::Error& ex) {
    std::cerr << "Registration of object failed." << std::endl;
  }
}

void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */, const Glib::ustring& /* name */)
{
}

void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
}

int main(int argc, char** argv)
{
  std::locale::global(std::locale(""));
  Gio::init();
  
  if (argc != 3) {
    std::cerr << "expecting two non-optional arguments <master-device> <slave-device>" << std::endl;
    return 1;
  }
  
  etherbone::Socket socket;
  etherbone::Device device;
  
  try {
    socket.open();
    // saftlib::WishboneDevices::probe(argc, argv);
    device.open(socket, argv[1]);
    socket.passive(argv[2]);
  } catch (const etherbone::exception_t& e) {
    std::cerr << e << std::endl;
    return 1;
  }
  
  // !!! FIXME, probe stuff intelligently
  saftlib::Devices devices;
  devices.push_back(saftlib::Device(device, 0x30000, 0x3ffff));
  saftlib::Device::hook_it_all(socket);
  
  const guint id = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
    "de.gsi.saftlib",
    sigc::ptr_fun(&on_bus_acquired),
    sigc::ptr_fun(&on_name_acquired),
    sigc::ptr_fun(&on_name_lost));
  
  Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
  eb_attach_source(loop, socket);
  
  saftlib::Drivers::start(devices);
  
  loop->run();
  
  saftlib::Drivers::stop();

  saftlib::ObjectRegistry::unregister_all();
  Gio::DBus::unown_name(id);
  
  device.close();
  socket.close();

  return 0;
}
