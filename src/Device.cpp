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
#include <iostream>
#include <iomanip>
#include <memory>

#include "Source.h"

namespace saftlib {

// Device::irqMap Device::irqs;   // this is in globals.cpp
// Device::msiQueue Device::msis; // this is in globals.cpp

Device::Device(etherbone::Device d, eb_address_t first, eb_address_t last, bool poll)
 : etherbone::Device(d), base(first), mask(last-first), activate_msi_polling(poll)
{
}

eb_address_t Device::request_irq(const etherbone::sdb_msi_device& sdb, const sigc::slot<void,eb_data_t>& slot)
{
  eb_address_t irq;
  
  // Confirm we had SDB records for MSI all the way down
  if (sdb.msi_last < sdb.msi_first) {
    throw etherbone::exception_t("request_irq/non_msi_crossbar_inbetween", EB_FAIL);
  }
  
  // Confirm that first is aligned to size
  eb_address_t size = sdb.msi_last - sdb.msi_first;
  if ((sdb.msi_first & size) != 0) {
    throw etherbone::exception_t("request_irq/misaligned", EB_FAIL);
  }
  
  // Confirm that the MSI range could contain our master (not mismapped)
  if (size < mask) {
    throw etherbone::exception_t("request_irq/badly_mapped", EB_FAIL);
  }
  
  // Select an IRQ
  int retry;
  for (retry = 1000; retry > 0; --retry) {
    irq = (rand() & mask) + base;
    irq &= ~7;
    if (irqs.find(irq) == irqs.end()) break;
  }
  
  if (retry == 0) {
    throw etherbone::exception_t("request_irq/no_free", EB_FAIL);
  }

  if (activate_msi_polling) {
    Slib::signal_timeout().connect(sigc::mem_fun(this, &Device::poll_msi), 1);
    activate_msi_polling = false;
  }
  
  msi_first = sdb.msi_first;
  // Bind the IRQ
  irqs[irq] = slot;
  // Report the MSI as seen from the point-of-view of the slave
  return irq + sdb.msi_first;
}

void Device::release_irq(eb_address_t irq)
{
  irqs.erase((irq & mask) + base);
}

bool Device::poll_msi() {
  //std::cerr << "polling for msi" << std::endl;
  etherbone::Cycle cycle;
  eb_data_t msi_adr = 0;
  eb_data_t msi_dat = 0;
  eb_data_t msi_cnt = 0;
  const int MAX_MSIS_IN_ONE_GO = 1024;
  for (int i = 0; i < MAX_MSIS_IN_ONE_GO; ++i) { // never more this many MSIs in one go
    cycle.open(*(etherbone::Device*)this);
    cycle.read_config(0x40, EB_DATA32, &msi_adr);
    cycle.read_config(0x44, EB_DATA32, &msi_dat);
    cycle.read_config(0x48, EB_DATA32, &msi_cnt);
    cycle.close();
    if (msi_cnt & 1) {
      Device::MSI msi;
      msi.address = msi_adr-msi_first;
      msi.data    = msi_dat;
      Device::msis.push_back(msi);
    }
    if (!(msi_cnt & 2)) {
      if (i) {
        //std::cerr << i << " msis popped" << std::endl;
      }
      break; // normal end 
    }
    if (i == MAX_MSIS_IN_ONE_GO-1) {
      std::cerr << "reached MAX_MSIS_IN_ONE_GO" << std::endl;
    }
  }
  return true;
}

struct IRQ_Handler : public etherbone::Handler
{
  eb_status_t read (eb_address_t address, eb_width_t width, eb_data_t* data);
  eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);
};

eb_status_t IRQ_Handler::read(eb_address_t address, eb_width_t width, eb_data_t* data)
{
  //std::cerr << "IRQ_Handler::read() " << std::endl;
  *data = 0;
  return EB_OK;
}

eb_status_t IRQ_Handler::write(eb_address_t address, eb_width_t width, eb_data_t data)
{
  //std::cerr << "IRQ_Handler::write() " << std::endl;
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

class MSI_Source : public Slib::Source
{
  public:
    static std::shared_ptr<MSI_Source> create();
    sigc::connection connect(const sigc::slot<bool>& slot);
  
  protected:
    explicit MSI_Source();

    virtual bool prepare(int& timeout);
    virtual bool check();
    virtual bool dispatch(sigc::slot_base* slot);

  private:
};


std::shared_ptr<MSI_Source> MSI_Source::create()
{
  return std::shared_ptr<MSI_Source>(new MSI_Source());
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
  //std::cerr << "MSI_Source::prepare" << std::endl;
  // returning true means immediately ready
  bool result;
  if (Device::msis.empty()) {
    result = false;
  } else {
    timeout_ms = 0;
    result = true;
  }
  //std::cerr << "MSI_Source::prepare(" << timeout_ms << ") " << this << " " << result << std::endl;
  return result;
}

bool MSI_Source::check()
{
  bool result = !Device::msis.empty(); // true means ready after glib's poll
  //std::cerr << "MSI_Source::check()    "<< result << std::endl;
  return result;
}

bool MSI_Source::dispatch(sigc::slot_base* slot)
{
  //std::cerr << "MSI_Source::dispatch() " << std::endl;
  // Don't process more than 10 MSIs in one go (must give dbus some service too)
  int limit = 10;
  
  // Process any pending MSIs
  while (!Device::msis.empty() && --limit) {
    Device::MSI msi = Device::msis.front();
    Device::msis.pop_front();
    
    Device::irqMap::iterator i = Device::irqs.find(msi.address);
    if (i != Device::irqs.end()) {
      try {
        //std::cerr  << "MSI_Source::dispatch() -> execute MSI " << msi.address << " " << msi.data << std::endl;
        i->second(msi.data);
      } catch (const etherbone::exception_t& ex) {
        std::cerr << "Unhandled etherbone exception in MSI handler for 0x" 
             << std::hex << msi.address << ": " << ex << std::endl;
      // } catch (const saftbus::Error& ex) {
      //   std::cerr << "Unhandled Glib exception in MSI handler for 0x" 
      //        << std::hex << msi.address << ": " << ex.what() << std::endl;
      } catch (std::exception& ex) {
        std::cerr << "Unhandled std::exception exception in MSI handler for 0x" 
             << std::hex << msi.address << ": " << ex.what() << std::endl;
      } catch (...) {
        std::cerr << "Unhandled unknown exception in MSI handler for 0x" 
             << std::hex << msi.address << std::endl;
      }
    } else {
      std::cerr << "No handler for MSI 0x" << std::hex << msi.address << std::endl;
    }
  }
  
  // return the signal handler
  return (*static_cast<sigc::slot<bool>*>(slot))();
}

static bool my_noop()
{
  return true;
}

sigc::connection Device::attach(const std::shared_ptr<Slib::MainLoop>& loop)
{
  std::shared_ptr<MSI_Source> source = MSI_Source::create();
  sigc::connection out = source->connect(sigc::ptr_fun(&my_noop));
  source->attach(loop->get_context(), source);
  return out;
}

} // namespace saftlib
