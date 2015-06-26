#define ETHERBONE_THROWS 1

#include <glibmm.h>
#include <cassert>
#include <iostream>

#include "SAFTd.h"
#include "Driver.h"
#include "eb-source.h"
#include "build.h"

namespace saftlib {

static void just_rethrow(const char*)
{
  throw;
}

SAFTd::SAFTd()
 : m_service(this, sigc::ptr_fun(&just_rethrow)), m_loop(Glib::MainLoop::create())
{
  // Setup the global etherbone socket
  try {
    socket.open();
    saftlib::Device::hook_it_all(socket);
    socket.passive("dev/wbs0"); // !!! remove this once dev/wbm0 and dev/wbs0 are unified
  } catch (const etherbone::exception_t& e) {
    std::cerr << "Failed to initialize etherbone: " << e << std::endl;
    exit(1);
  }
  
  // Connect etherbone to glib loop
  sigc::connection eb_source = eb_attach_source(m_loop, socket);
}

SAFTd::~SAFTd()
{
  try {
    for (std::map< Glib::ustring, OpenDevice >::iterator i = devs.begin(); i != devs.end(); ++i) {
      i->second.ref.clear(); // should destroy the driver
      i->second.device.close();
    }
    devs.clear();
    
    if (m_connection) {
      m_service.unregister_self();
      m_connection.reset();
    }
    eb_source.disconnect();
    socket.close();
  } catch (const Glib::Error& ex) {
    std::cerr << "Could not clean up: " << ex.what() << std::endl;
    exit(1);
  } catch(const etherbone::exception_t& ex) {
    std::cerr << "Could not clean up: " << ex << std::endl;
    exit(1);
  }
  
  std::cout << "Clean shutdown" << std::endl;
}

void SAFTd::Quit()
{
  m_loop->quit();
}

std::map< Glib::ustring, Glib::ustring > SAFTd::getDevices() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  for (std::map< Glib::ustring, OpenDevice >::const_iterator i = devs.begin(); i != devs.end(); ++i) {
    out[i->first] = i->second.objectPath;
  }
  return out;
}

void SAFTd::setConnection(const Glib::RefPtr<Gio::DBus::Connection>& c)
{
  assert (!m_connection);
  m_connection = c;
  m_service.register_self(m_connection, "/de/gsi/saftlib");
}

static inline bool not_isalnum_(char c) 
{ 
  return !(isalnum(c) || c == '_');
}

Glib::ustring SAFTd::AttachDevice(const Glib::ustring& name, const Glib::ustring& path)
{
  if (devs.find(name) != devs.end())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "device already exists");
  if (find_if(name.begin(), name.end(), not_isalnum_) != name.end())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
  
  etherbone::Device edev;
  try {
    edev.open(socket, path.c_str());
  } catch (const etherbone::exception_t& e) {
    std::ostringstream str;
    str << "AttachDevice: failed to open: " << e;
    throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, str.str().c_str());
  }
  
  struct OpenDevice od(edev);
  od.name = name;
  od.objectPath = "/de/gsi/saftlib/" + name;
  od.etherbonePath = path;
  
  try {
    Drivers::probe(od);
  } catch (const etherbone::exception_t& ex) {
    std::ostringstream str;
    str << "AttachDevice: failed to probe: " << ex;
    throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, str.str().c_str());
  } catch (...) {
    edev.close();
    throw;
  }
  
  if (od.ref) {
    devs.insert(std::make_pair(name, od));
    // inform clients of updated property
    Devices(getDevices());
    return od.objectPath;
  } else {
    edev.close();
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "no driver available for this device");
  }
}

void SAFTd::RemoveDevice(const Glib::ustring& name)
{
  std::map< Glib::ustring, OpenDevice >::iterator elem = devs.find(name);
  if (elem == devs.end())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "no such device");
  
  elem->second.ref.clear();
  elem->second.device.close();
  devs.erase(elem);
  
  // inform clients of updated property 
  Devices(getDevices());
}

Glib::ustring SAFTd::getSourceVersion() const
{
  return sourceVersion;
}

Glib::ustring SAFTd::getBuildInfo() const
{
  return buildInfo;
}

} // saftlib
