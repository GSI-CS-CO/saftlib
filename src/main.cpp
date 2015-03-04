#define ETHERBONE_THROWS 1

#include <iostream>
#include <giomm.h>
#include <glibmm.h>
#include <signal.h>

#include "Directory.h"
#include "eb-source.h"

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  try {
    saftlib::Directory::get()->setConnection(connection);
  } catch(const Glib::Error& ex) {
//    std::cerr << "Could not create directory: " << ex << std::endl;
    exit(1);
  } catch(const etherbone::exception_t& ex) {
    std::cerr << "Could not create directory: " << ex << std::endl;
    exit(1);
  }
}

void on_sigint(int)
{
  saftlib::Directory::get()->loop()->quit();
}

void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */, const Glib::ustring& /* name */)
{
  std::cout << "Up and running" << std::endl;
}

void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  // Something else claimed the saftlib name
  saftlib::Directory::get()->loop()->quit();
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
    saftlib::Device::hook_it_all(socket);
    device.open(socket, argv[1]);
    saftlib::Directory::get()->devices().push_back(device);
    socket.passive(argv[2]); // !!! remove once interrupts come via master
  } catch (const etherbone::exception_t& e) {
    std::cerr << e << std::endl;
    return 1;
  }
  
  // Connect etherbone to glib loop
  sigc::connection eb_source = 
    eb_attach_source(saftlib::Directory::get()->loop(), socket);
  
  const guint id = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
    "de.gsi.saftlib",
    sigc::ptr_fun(&on_bus_acquired),
    sigc::ptr_fun(&on_name_acquired),
    sigc::ptr_fun(&on_name_lost));
  
  // Catch control-C for clean shutdown
  signal(SIGINT,  &on_sigint);
  signal(SIGTERM, &on_sigint);
  signal(SIGQUIT, &on_sigint);
  
  saftlib::Directory::get()->loop()->run();
  
  Gio::DBus::unown_name(id);
  saftlib::Directory::get()->resetConnection();
  //eb_source.disconnect();
  
  device.close();
  socket.close();

  return 0;
}
