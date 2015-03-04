#define ETHERBONE_THROWS 1

#include <glibmm.h>
#include "Directory.h"
#include "Driver.h"

namespace saftlib {

Directory::Directory()
 : Directory_Service(), m_loop(Glib::MainLoop::create())
{
}

Directory::~Directory()
{
}

const Glib::RefPtr<Directory>& Directory::get()
{
  static Glib::RefPtr<Directory> top;
  if (!top) top = Glib::RefPtr<Directory>(new Directory);
  return top;
}

void Directory::Quit()
{
  m_loop->quit();
}

void Directory::setConnection(const Glib::RefPtr<Gio::DBus::Connection>& c)
{
  m_connection = c;
  Drivers::probe();
  setDevices(devs);
  register_self(m_connection, "/de/gsi/saftlib/Directory");
}

void Directory::resetConnection()
{
  unregister_self();
  devs.clear();
  setDevices(devs);
  refs.clear(); // destroys and unregisters all child objects
  m_connection.reset();
}

void Directory::add(const Glib::ustring& iface, const Glib::ustring& path, const Glib::RefPtr<Glib::Object>& object)
{
  devs[iface].push_back(path);
  refs.push_back(object);
}

} // saftlib
