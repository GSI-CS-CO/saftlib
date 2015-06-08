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

void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */, const Glib::ustring& /* name */, int argc, char** argv)
{
  for (int i = 1; i < argc; ++i) {
    // parse the string
    std::string command = argv[i];
    std::string::size_type pos = command.find_first_of(':');
    if (pos == std::string::npos) {
      std::cerr << "Argument '" << command << "' is not of form <logical-name>:<etherbone-path>" << std::endl;
      exit(1);
    }
    
    std::string name = command.substr(0, pos);
    std::string path = command.substr(pos+1, std::string::npos);
    saftlib::Directory::get()->AttachDevice(name, path);
  }
  
  // std::cout << "Up and running" << std::endl;
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
  
  if (argc < 2) {
    std::cerr << "expecting at least one argument <logical-name>:<etherbone-path> ..." << std::endl;
    return 1;
  }
  
  etherbone::Socket socket;
  try {
    socket.open();
    saftlib::Device::hook_it_all(socket);
    socket.passive("dev/wbs0"); // !!! remove this once dev/wbm0 and dev/wbs0 are unified
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
    sigc::bind(sigc::bind(sigc::ptr_fun(&on_name_acquired), argv), argc),
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
