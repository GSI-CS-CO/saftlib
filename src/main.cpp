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
    std::cerr << "Could not create directory: " << ex.what() << std::endl;
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

void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */, const Glib::ustring& /* name */, char** argv)
{
  std::cout << "Up and running" << std::endl;
}

void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  // Something else claimed the saftlib name
  std::cerr << "Unable to acquire name---dbus saftlib.conf installed?" << std::endl;
  saftlib::Directory::get()->loop()->quit();
}

int main(int argc, char** argv)
{
  std::locale::global(std::locale(""));
  Gio::init();
  
  if (argc != 4) {
    std::cerr << "expecting three non-optional arguments <logical-name> <master-device> <slave-device>" << std::endl;
    return 1;
  }
  
  etherbone::Socket socket;
  try {
    socket.open();
    saftlib::Device::hook_it_all(socket);
//    saftlib::Directory::get()->AttachDevice(argv[1], argv[2]);
//    socket.passive(argv[3]);
  } catch (const etherbone::exception_t& e) {
    std::cerr << e << std::endl;
    return 1;
  }
  
  // Connect etherbone to glib loop
  sigc::connection eb_source = 
    eb_attach_source(saftlib::Directory::get()->loop(), socket);
  
  const guint id = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SYSTEM,
    "de.gsi.saftlib",
    sigc::ptr_fun(&on_bus_acquired),
    sigc::bind(sigc::ptr_fun(&on_name_acquired), argv),
    sigc::ptr_fun(&on_name_lost));
  
  // Catch control-C for clean shutdown
  signal(SIGINT,  &on_sigint);
  signal(SIGTERM, &on_sigint);
  signal(SIGQUIT, &on_sigint);
  
  // Run the main event loop
  saftlib::Directory::get()->loop()->run();
  
  // Cleanup
  Gio::DBus::unown_name(id);
  saftlib::Directory::get()->resetConnection();
  eb_source.disconnect();
  socket.close();

  return 0;
}
