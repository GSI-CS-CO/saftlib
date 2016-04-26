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

#include <glibmm.h>
#include <cassert>
#include <iostream>

#include "SAFTd.h"
#include "Driver.h"
#include "eb-source.h"
#include "build.h"
#include "clog.h"

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
    // still attached to cerr at this point
    std::cerr << "Failed to initialize etherbone: " << e << std::endl;
    exit(1);
  }
  
  // Connect etherbone to glib loop
  eb_source = eb_attach_source(m_loop, socket);
  // Connect the IRQ buffer to glip loop
  msi_source = Device::attach(m_loop);
}

SAFTd::~SAFTd()
{
  bool daemon = false;
  try {
    for (std::map< Glib::ustring, OpenDevice >::iterator i = devs.begin(); i != devs.end(); ++i) {
      i->second.ref.clear(); // should destroy the driver
      i->second.device.close();
    }
    devs.clear();
    
    if (m_connection) {
      daemon = true;
      m_service.unregister_self();
      m_connection.reset();
    }
    eb_source.disconnect();
    msi_source.disconnect();
    socket.close();
  } catch (const Glib::Error& ex) {
    clog << kLogErr << "Could not clean up: " << ex.what() << std::endl;
    exit(1);
  } catch(const etherbone::exception_t& ex) {
    clog << kLogErr << "Could not clean up: " << ex << std::endl;
    exit(1);
  }
  
  if (daemon) clog << kLogNotice << "shutdown" << std::endl;
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

struct probe_root {
  eb_address_t first;
  eb_address_t last;
  eb_status_t status;
};

void probe_root_cb(eb_user_data_t user, eb_device_t, const struct sdb_table*, eb_address_t msi_first, eb_address_t msi_last, eb_status_t status)
{
  probe_root* out = reinterpret_cast<probe_root*>(user);
  out->first = msi_first;
  out->last = msi_last;
  out->status = status;
}

Glib::ustring SAFTd::AttachDevice(const Glib::ustring& name, const Glib::ustring& path)
{
  if (devs.find(name) != devs.end())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "device already exists");
  if (find_if(name.begin(), name.end(), not_isalnum_) != name.end())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
  
  try {
    etherbone::Device edev;
    edev.open(socket, path.c_str());
    
    // Determine the size of the MSI range
    probe_root probe;
    probe.status = 1;
    edev.sdb_scan_root_msi(&probe, &probe_root_cb);
    do socket.run(); while (probe.status == 1);
    
    if (probe.status != EB_OK)
      throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, Glib::ustring("Could not scan bus root: ") + eb_status(probe.status));
    
    if (probe.last < probe.first)
      throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, "Device does not support MSI");
    
    // In case we are one master among many, mark us at 0
    probe.last -= probe.first;
    probe.first = 0;
    
    // Confirm the size is a power of 2 (so we can mask easily)
    if (((probe.last + 1) & probe.last) != 0)
      throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, "Device has strange sized MSI range");
    
    struct OpenDevice od(edev, probe.last);
    od.name = name;
    od.objectPath = "/de/gsi/saftlib/" + name;
    od.etherbonePath = path;
    
    try {
      Drivers::probe(od);
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
  } catch (const etherbone::exception_t& e) {
    std::ostringstream str;
    str << "AttachDevice: failed to open: " << e;
    throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, str.str().c_str());
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
