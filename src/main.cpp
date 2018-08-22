/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <iostream>
#include <giomm.h>
#include <glibmm.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h> // GNU extension for backtrace
#include <cxxabi.h>   // GCC extension for __cxa_demangle

#include "SAFTd.h"
#include "clog.h"
#include "build.h"

using namespace saftlib;

static bool am_daemon = false;

static void print_backtrace(std::ostream& stream, const char *where)
{
  stream << where << ": ";
  
  try {
    throw;
  } catch (const std::exception &ex) {
    stream << "std::exception: " << ex.what() << "\n";
  } catch(const Glib::Error& ex) {
    stream << "Glib::Error: " << ex.what() << "\n";
  } catch(const etherbone::exception_t& ex) {
    stream << "etherbone::exception_t: " << ex << "\n";
  } catch(...) {
    stream << "unknown exception\n";
  }
  
  void * array[50];
  int size = backtrace(array, sizeof(array)/sizeof(array[0]));
  char ** messages = backtrace_symbols(array, size);
  
  if (messages) {
    stream << "Stack-trace:\n";
    for (int i = 1; i < size; ++i) { // Skip 0 = this function
      std::string line(messages[i]);
      // Demangle the symbols
      int status;
      std::string::size_type end   = line.rfind('+');
      std::string::size_type start = line.rfind('(');
      std::string symbol(line, start+1, end-start-1);
      char *demangle = abi::__cxa_demangle(symbol.c_str(), 0, 0, &status);
      if (status == 0) {
        stream << "  " << line.substr(0, start+1) << demangle << line.substr(end) << "\n";
        free(demangle);
      } else {
        stream << "  " << messages[i] << "\n";
      }
    }
    stream << std::flush;
    free(messages);
  } else {
    stream << "Unable to generate stack trace" << std::endl;
  }
  
  abort();
}

// Handle uncaught exceptions
static void my_terminate()
{
  print_backtrace(am_daemon ? (clog << kLogErr) : std::cerr, "Unhandled exception ");
}

static void on_bus_acquired(const Glib::RefPtr<saftbus::Connection>& connection, const Glib::ustring& /* name */)
{
  try {
    SAFTd::get().setConnection(connection);
  } catch (...) {
    print_backtrace(std::cerr, "Could not setConnection");
  }
}

static void on_name_acquired(const Glib::RefPtr<saftbus::Connection>& /* connection */, const Glib::ustring& /* name */, int argc, char** argv)
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
      SAFTd::get().AttachDevice(name, path);
    } catch (...) {
      print_backtrace(std::cerr, ("Attaching " + name + "(" + path + ")").c_str());
      throw;
    }
  }
  
  // startup complete; detach from terminal
  int devnull_w = open("/dev/null", O_WRONLY);
  int devnull_r = open("/dev/null", O_RDONLY);
  if (devnull_w == -1 || devnull_r == -1) {
    std::cerr << "failed to open /dev/null" << std::endl;
    exit (1);
  }
  if (dup2(devnull_r, 0) == -1 || 
      dup2(devnull_w, 1) == -1 ||
      dup2(devnull_w, 2) == -1) {
    std::cerr << "failed to close stdin/stdout/stderr" << std::endl;
  }
  close(devnull_r);
  close(devnull_w);
  
  am_daemon = true;
  
  // log success
  clog << kLogNotice << "started" << std::endl;
  clog << kLogInfo << "sourceVersion: " << sourceVersion << std::endl;
  clog << kLogInfo << buildInfo << std::endl;
}

static void on_name_lost(const Glib::RefPtr<saftbus::Connection>& connection, const Glib::ustring& /* name */)
{
  // Something else claimed the saftlib name
  (am_daemon ? (clog << kLogErr) : std::cerr) << "Unable to acquire name---dbus saftlib.conf installed?" << std::endl;
  SAFTd::get().loop()->quit();
}

static void on_sigint(int)
{
  SAFTd::get().loop()->quit();
}

int main(int argc, char** argv)
{
  // Hook a stack tracer up to exceptions
  std::set_terminate(my_terminate);
  
  // Catch signals for clean shutdown
  signal(SIGINT,  &on_sigint);
  signal(SIGTERM, &on_sigint);
  signal(SIGQUIT, &on_sigint);
  signal(SIGHUP,  SIG_IGN);
  
  if (chdir("/") == -1) {
    std::cerr << "failed to leave current directory" << std::endl;
    return 1;
  }
  
  // turn into a daemon
  switch (fork()) {
  case -1: std::cerr << "failed to fork" << std::endl; exit(1);
  case 0:  break;
  default: exit(0);
  }
  
  // leave current session once we fork again
  setsid();
  
  // second fork ensures we are an orphan
  switch (fork()) {
  case -1: std::cerr << "failed to fork" << std::endl; exit(1);
  case 0:  break;
  default: exit(0);
  }

  // initialize gio
  std::locale::global(std::locale(""));
  Gio::init();
  
  // Connect to the dbus system daemon
  const guint id = saftbus::own_name(saftbus::BUS_TYPE_SYSTEM,
    "de.gsi.saftlib",
    sigc::ptr_fun(&on_bus_acquired),
    sigc::bind(sigc::bind(sigc::ptr_fun(&on_name_acquired), argv), argc),
    sigc::ptr_fun(&on_name_lost));
  
  // Run the main event loop
  std::cerr << "Saftd main loop running" << std::endl;
  SAFTd::get().loop()->run();
  
  // Cleanup
  saftbus::unown_name(id);

  return 0;
}
