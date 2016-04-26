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

#include <stdlib.h>
#include <string.h>
#include "Device.h"
#include "clog.h"

namespace saftlib {

Device::Device(etherbone::Device d, eb_address_t m)
 : etherbone::Device(d), mask(m)
{
}

eb_address_t Device::request_irq(const etherbone::sdb_msi_device& sdb, const sigc::slot<void,eb_data_t>& slot)
{
  eb_address_t irq;
  
  // Confirm that our MSI range matches what was probed
  if (sdb.msi_last - sdb.msi_first != mask) {
    throw etherbone::exception_t("request_irq/wrong_irq", EB_FAIL);
  }
  
  // Confirm that first is a power of two
  if (((sdb.msi_first - 1) & sdb.msi_first) != 0) {
    throw etherbone::exception_t("request_irq/misaligned", EB_FAIL);
  }
  
  // Select an IRQ
  int retry;
  for (retry = 1000; retry > 0; --retry) {
    irq = (rand() & mask);
    irq &= ~7;
    if (irqs.find(irq) == irqs.end()) break;
  }
  
  if (retry == 0) {
    throw etherbone::exception_t("request_irq/no_free", EB_FAIL);
  }
  
  // Bind the IRQ
  irqs[irq] = slot;
  // Report the MSI as seen from the point-of-view of the slave
  return irq + sdb.msi_first;
}

void Device::release_irq(eb_address_t irq)
{
  irqs.erase(irq & mask);
}

struct IRQ_Handler : public etherbone::Handler
{
  eb_status_t read (eb_address_t address, eb_width_t width, eb_data_t* data);
  eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);
};

eb_status_t IRQ_Handler::read(eb_address_t address, eb_width_t width, eb_data_t* data)
{
  *data = 0;
  return EB_OK;
}

eb_status_t IRQ_Handler::write(eb_address_t address, eb_width_t width, eb_data_t data)
{
  Device::MSI msi;
  msi.address = address;
  msi.data = data;
  Device::msis.push_back(msi);
  return EB_OK;
}

void Device::hook_it_all(etherbone::Socket socket)
{
  static sdb_device everything;
  static IRQ_Handler handler;
  
  everything.abi_class     = 0;
  everything.abi_ver_major = 0;
  everything.abi_ver_minor = 0;
  everything.bus_specific  = SDB_WISHBONE_WIDTH;
  everything.sdb_component.addr_first = 0;
  everything.sdb_component.addr_last  = UINT32_C(0xffffffff);
  everything.sdb_component.product.vendor_id = 0x651;
  everything.sdb_component.product.device_id = 0xefaa70;
  everything.sdb_component.product.version   = 1;
  everything.sdb_component.product.date      = 0x20150225;
  memcpy(everything.sdb_component.product.name, "SAFTLIB           ", 19);
  
  socket.attach(&everything, &handler);
}

class MSI_Source : public Glib::Source
{
  public:
    static Glib::RefPtr<MSI_Source> create();
    sigc::connection connect(const sigc::slot<bool>& slot);
    
  protected:
    explicit MSI_Source();
    
    virtual bool prepare(int& timeout);
    virtual bool check();
    virtual bool dispatch(sigc::slot_base* slot);
};

Glib::RefPtr<MSI_Source> MSI_Source::create()
{
  return Glib::RefPtr<MSI_Source>(new MSI_Source());
}

sigc::connection MSI_Source::connect(const sigc::slot<bool>& slot)
{
  return connect_generic(slot);
}

MSI_Source::MSI_Source()
{
}

bool MSI_Source::prepare(int& timeout_ms)
{
  // returning true means immediately ready
  if (Device::msis.empty()) {
    return false;
  } else {
    timeout_ms = 0;
    return true;
  }
}

bool MSI_Source::check()
{
  return !Device::msis.empty(); // true means ready after glib's poll
}

bool MSI_Source::dispatch(sigc::slot_base* slot)
{
  // Don't process more than 10 MSIs in one go (must give dbus some service too)
  int limit = 10;
  
  // Process any pending MSIs
  while (!Device::msis.empty() && --limit) {
    Device::MSI msi = Device::msis.front();
    Device::msis.pop_front();
    
    msi.address &= 0x7fffffffUL; // !!! work-around for ftm crossbar bug
    Device::irqMap::iterator i = Device::irqs.find(msi.address);
    if (i != Device::irqs.end()) {
      try {
        i->second(msi.data);
      } catch (const etherbone::exception_t& ex) {
        clog << kLogErr << "Unhandled etherbone exception in MSI handler for 0x" 
             << std::hex << msi.address << ": " << ex << std::endl;
      } catch (const Glib::Error& ex) {
        clog << kLogErr << "Unhandled Glib exception in MSI handler for 0x" 
             << std::hex << msi.address << ": " << ex.what() << std::endl;
      } catch (std::exception& ex) {
        clog << kLogErr << "Unhandled std::exception exception in MSI handler for 0x" 
             << std::hex << msi.address << ": " << ex.what() << std::endl;
      } catch (...) {
        clog << kLogErr << "Unhandled unknown exception in MSI handler for 0x" 
             << std::hex << msi.address << std::endl;
      }
    } else {
      clog << kLogErr << "No handler for MSI 0x" << std::hex << msi.address << std::endl;
    }
  }
  
  // return the signal handler
  return (*static_cast<sigc::slot<bool>*>(slot))();
}

static bool my_noop()
{
  return true;
}

sigc::connection Device::attach(const Glib::RefPtr<Glib::MainLoop>& loop)
{
  Glib::RefPtr<MSI_Source> source = MSI_Source::create();
  sigc::connection out = source->connect(sigc::ptr_fun(&my_noop));
  source->attach(loop->get_context());
  return out;
}

} // saftlib
