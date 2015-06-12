#define ETHERBONE_THROWS 1

#include <iostream>
#include <giomm.h>
#include <glibmm.h>
#include <signal.h>

#include "SAFTd.h"

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  try {
    saftlib::SAFTd::get()->setConnection(connection);
  } catch(const Glib::Error& ex) {
    std::cerr << "Could not create directory: " << ex.what() << std::endl;
    exit(1);
  } catch(const etherbone::exception_t& ex) {
    std::cerr << "Could not create directory: " << ex << std::endl;
    exit(1);
  }
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
    try {
      saftlib::SAFTd::get()->AttachDevice(name, path);
    } catch (const Glib::Error& ex) {
      std::cerr << "Could not open device " << path << ": " << ex.what() << std::endl;
      exit(1);
    } catch(const etherbone::exception_t& ex) {
      std::cerr << "Could not open device " << path << ": " << ex << std::endl;
      exit(1);
    }
  }
  
  std::cout << "Up and running" << std::endl;
}

void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  // Something else claimed the saftlib name
  std::cerr << "Unable to acquire name---dbus saftlib.conf installed?" << std::endl;
  saftlib::SAFTd::get()->loop()->quit();
}

void on_sigint(int)
{
  saftlib::SAFTd::get()->loop()->quit();
}

int main(int argc, char** argv)
{
  std::locale::global(std::locale(""));
  Gio::init();
  
  if (argc < 2) {
    std::cerr << "expecting at least one argument <logical-name>:<etherbone-path> ..." << std::endl;
    return 1;
  }
  
  // Connect to the dbus system daemon
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
  saftlib::SAFTd::get()->loop()->run();
  
  // Cleanup
  Gio::DBus::unown_name(id);

  return 0;
}
