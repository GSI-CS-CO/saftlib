#define ETHERBONE_THROWS 1

#include <glibmm.h>
#include "Directory.h"
#include "Driver.h"

namespace saftlib {

Directory::Directory()
 : Directory_Service(), m_loop(Glib::MainLoop::create())
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
  if (m_connection) resetConnection();
  m_connection = c;
  // !!! objects.register_self
  register_self(m_connection, "/de/gsi/saftlib/Directory");
}

void Directory::resetConnection()
{
  // !!! objects.unregister_self
  unregister_self();
  m_connection.reset();
}

void Directory::AttachDevice(const Glib::ustring& name, const Glib::ustring& path)
{
  // !!!
  // Drivers::probe();
}

} // saftlib
