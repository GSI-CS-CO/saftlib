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

//#include <glibmm.h>
#include <cassert>
#include <iostream>
#include <algorithm>

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
 : m_service(this, sigc::ptr_fun(&just_rethrow)), m_loop(Slib::MainLoop::create())
{
  // Setup the global etherbone socket
  try {
    socket.open();
    saftlib::Device::hook_it_all(socket);
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
    for (std::map< std::string, OpenDevice >::iterator i = devs.begin(); i != devs.end(); ++i) {
      i->second.ref.reset(); // should destroy the driver
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
  } catch (const saftbus::Error& ex) {
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

std::map< std::string, std::string > SAFTd::getDevices() const
{
  std::map< std::string, std::string > out;
  for (std::map< std::string, OpenDevice >::const_iterator i = devs.begin(); i != devs.end(); ++i) {
    out[i->first] = i->second.objectPath;
  }
  return out;
}

void SAFTd::setConnection(const std::shared_ptr<saftbus::Connection>& c)
{
  assert (!m_connection);
  m_connection = c;
  m_service.register_self(m_connection, "/de/gsi/saftlib");
}

static inline bool not_isalnum_(char c) 
{ 
  return !(isalnum(c) || c == '_');
}

std::string SAFTd::AttachDevice(const std::string& name, const std::string& path)
{
  if (devs.find(name) != devs.end())
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "device already exists");
  if (find_if(name.begin(), name.end(), not_isalnum_) != name.end())
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
  
  try {
    etherbone::Device edev;
    edev.open(socket, path.c_str());
    
    try {
      // Determine the size of the MSI range
      eb_address_t first, last;
      edev.enable_msi(&first, &last);
    
      // Confirm the device is an aligned power of 2
      eb_address_t size = last - first;
    
      if (((size + 1) & size) != 0)
        throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has strange sized MSI range");
    
      if ((first & size) != 0)
        throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has unaligned MSI first address");
    
      struct OpenDevice od(edev, first, last);
      od.name = name;
      od.objectPath = "/de/gsi/saftlib/" + name;
      od.etherbonePath = path;
    
      Drivers::probe(od);
      if (od.ref) {
        devs.insert(std::make_pair(name, od));
        // inform clients of updated property
        Devices(getDevices());
        return od.objectPath;
      } else {
        throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no driver available for this device");
      }
    } catch (...) {
      edev.close();
      throw;
    }
  } catch (const etherbone::exception_t& e) {
    std::ostringstream str;
    str << "AttachDevice: failed to open: " << e;
    throw saftbus::Error(saftbus::Error::IO_ERROR, str.str().c_str());
  }
}

void SAFTd::RemoveDevice(const std::string& name)
{
  std::map< std::string, OpenDevice >::iterator elem = devs.find(name);
  if (elem == devs.end())
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such device");
  
  elem->second.ref.reset();
  elem->second.device.close();
  devs.erase(elem);
  
  // inform clients of updated property 
  Devices(getDevices());
}

std::string SAFTd::getSourceVersion() const
{
  return sourceVersion;
}

std::string SAFTd::getBuildInfo() const
{
  return buildInfo;
}

} // saftlib
