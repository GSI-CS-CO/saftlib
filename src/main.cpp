#define ETHERBONE_THROWS 1

#include <iostream>
#include <giomm.h>
#include <glibmm.h>
#include <signal.h>
#include <execinfo.h> // GNU extension for backtrace
#include <cxxabi.h>   // GCC extension for __cxa_demangle

#include "SAFTd.h"

void print_backtrace(const char *where)
{
  std::cerr << where << ": ";
  
  try {
    throw;
  } catch (const std::exception &ex) {
    std::cerr << "std::exception: " << ex.what() << std::endl;
  } catch(const Glib::Error& ex) {
    std::cerr << "Glib::Error: " << ex.what() << std::endl;
  } catch(const etherbone::exception_t& ex) {
    std::cerr << "etherbone::exception_t: " << ex << std::endl;
  } catch(...) {
    std::cerr << "unknown exception" << std::endl;
  }
  
  void * array[50];
  int size = backtrace(array, sizeof(array)/sizeof(array[0]));
  char ** messages = backtrace_symbols(array, size);
  
  if (messages) {
    std::cerr << "Stack-trace:\n" ;
    for (int i = 1; i < size; ++i) { // Skip 0 = this function
      std::string line(messages[i]);
      // Demangle the symbols
      int status;
      std::string::size_type end   = line.rfind('+');
      std::string::size_type start = line.rfind('(');
      std::string symbol(line, start+1, end-start-1);
      char *demangle = abi::__cxa_demangle(symbol.c_str(), 0, 0, &status);
      if (status == 0) {
        std::cerr << "  " << line.substr(0, start+1) << demangle << line.substr(end) << std::endl;
        free(demangle);
      } else {
        std::cerr << "  " << messages[i] << std::endl;
      }
    }
    free(messages);
  } else {
    std::cerr << "Unable to generate stack trace" << std::endl;
  }
  
  abort();
}

// Handle uncaught exceptions
void my_terminate() { print_backtrace("Unhandled exception "); }
static const bool SET_TERMINATE = std::set_terminate(my_terminate);

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  try {
    saftlib::SAFTd::get().setConnection(connection);
  } catch (...) {
    print_backtrace("Could not setConnection");
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
      saftlib::SAFTd::get().AttachDevice(name, path);
    } catch (...) {
      print_backtrace(("Attaching " + name + "(" + path + ")").c_str());
      throw;
    }
  }
  
  std::cout << "Up and running" << std::endl;
}

void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& /* name */)
{
  // Something else claimed the saftlib name
  std::cerr << "Unable to acquire name---dbus saftlib.conf installed?" << std::endl;
  saftlib::SAFTd::get().loop()->quit();
}

void on_sigint(int)
{
  saftlib::SAFTd::get().loop()->quit();
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
  saftlib::SAFTd::get().loop()->run();
  
  // Cleanup
  Gio::DBus::unown_name(id);

  return 0;
}
