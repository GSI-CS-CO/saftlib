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

#include "Device.hpp"

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <memory>


namespace eb_plugin {

// Device::irqMap Device::irqs;   // this is in globals.cpp
// Device::msiQueue Device::msis; // this is in globals.cpp

Device::Device(etherbone::Device d, eb_address_t first, eb_address_t last, bool poll, unsigned piv)
 : etherbone::Device(d), base(first), mask(last-first), activate_msi_polling(poll), polling_interval_ms(piv)
{
}

// eb_address_t Device::request_irq(const etherbone::sdb_msi_device& sdb, const sigc::slot<void,eb_data_t>& slot)
// {
//   eb_address_t irq;
  
//   // Confirm we had SDB records for MSI all the way down
//   if (sdb.msi_last < sdb.msi_first) {
//     throw etherbone::exception_t("request_irq/non_msi_crossbar_inbetween", EB_FAIL);
//   }
  
//   // Confirm that first is aligned to size
//   eb_address_t size = sdb.msi_last - sdb.msi_first;
//   if ((sdb.msi_first & size) != 0) {
//     throw etherbone::exception_t("request_irq/misaligned", EB_FAIL);
//   }
  
//   // Confirm that the MSI range could contain our master (not mismapped)
//   if (size < mask) {
//     throw etherbone::exception_t("request_irq/badly_mapped", EB_FAIL);
//   }
  
//   // Select an IRQ
//   int retry;
//   for (retry = 1000; retry > 0; --retry) {
//     irq = (rand() & mask) + base;
//     irq &= ~7;
//     if (irqs.find(irq) == irqs.end()) break;
//   }
  
//   if (retry == 0) {
//     throw etherbone::exception_t("request_irq/no_free", EB_FAIL);
//   }

//   if (activate_msi_polling) {
//     Slib::signal_timeout().connect(sigc::mem_fun(this, &Device::poll_msi), polling_interval_ms);
//     activate_msi_polling = false;
//   }
  
//   msi_first = sdb.msi_first;
//   // Bind the IRQ
//   irqs[irq] = slot;
//   // Report the MSI as seen from the point-of-view of the slave
//   return irq + sdb.msi_first;
// }

// void Device::release_irq(eb_address_t irq)
// {
//   irqs.erase((irq & mask) + base);
// }

// bool Device::poll_msi() {
//   // DRIVER_LOG("USB-poll-for-MSIs",-1,-1); 
//   etherbone::Cycle cycle;
//   eb_data_t msi_adr = 0;
//   eb_data_t msi_dat = 0;
//   eb_data_t msi_cnt = 0;
//   bool found_msi = false;
//   const int MAX_MSIS_IN_ONE_GO = 3; // not too many MSIs at once to not block saftd 
//   for (int i = 0; i < MAX_MSIS_IN_ONE_GO; ++i) { // never more this many MSIs in one go
//     cycle.open(*(etherbone::Device*)this);
//     cycle.read_config(0x40, EB_DATA32, &msi_adr);
//     cycle.read_config(0x44, EB_DATA32, &msi_dat);
//     cycle.read_config(0x48, EB_DATA32, &msi_cnt);
//     cycle.close();
//     if (msi_cnt & 1) {
//       Device::MSI msi;
//       msi.address = msi_adr-msi_first;
//       msi.data    = msi_dat;
//       Device::msis.push_back(msi);
//       found_msi = true;
//     }
//     if (!(msi_cnt & 2)) { // no more msi to poll
//       break; 
//     }
//   }
//   if ((msi_cnt & 2) || found_msi) {
//     // if we polled MAX_MSIS_IN_ONE_GO but there are more MSIs
//     // OR if there was at least one MSI present 
//     // we have to schedule the next check immediately because the 
//     // MSI we just polled may cause actions that trigger other MSIs.
//     saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
//     		std::bind(Device::poll_msi, *this), std::chrono::milliseconds(0)
//     	);
//   } else {
//     // if there was no MSI present we continue with the normal polling schedule
//     saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
//     		std::bind(Device::poll_msi, *this), std::chrono::milliseconds(polling_interval_ms)
//     	);
//   }
//   DRIVER_LOG("USB-poll-for-MSIs done",-1,-1); 

//   return false;
// }

// struct EbSlaveHandler : public etherbone::Handler
// {
//   eb_status_t read (eb_address_t address, eb_width_t width, eb_data_t* data);
//   eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);
// };

// eb_status_t EbSlaveHandler::read(eb_address_t address, eb_width_t width, eb_data_t* data)
// {
//   *data = 0;
//   return EB_OK;
// }

// eb_status_t EbSlaveHandler::write(eb_address_t address, eb_width_t width, eb_data_t data)
// {
//   Device::MSI msi;
//   msi.address = address;
//   msi.data = data;
//   Device::msis.push_back(msi);
//   return EB_OK;
// }

// void Device::hook_it_all(etherbone::Socket socket)
// {
//   static sdb_device     eb_slave_sdb;
//   static EbSlaveHandler eb_slave_handler;
  
//   eb_slave_sdb.abi_class     = 0;
//   eb_slave_sdb.abi_ver_major = 0;
//   eb_slave_sdb.abi_ver_minor = 0;
//   eb_slave_sdb.bus_specific  = SDB_WISHBONE_WIDTH;
//   eb_slave_sdb.sdb_component.addr_first = 0;
//   eb_slave_sdb.sdb_component.addr_last  = UINT32_C(0xffffffff);
//   eb_slave_sdb.sdb_component.product.vendor_id = 0x651;
//   eb_slave_sdb.sdb_component.product.device_id = 0xefaa70;
//   eb_slave_sdb.sdb_component.product.version   = 1;
//   eb_slave_sdb.sdb_component.product.date      = 0x20150225;
//   memcpy(eb_slave_sdb.sdb_component.product.name, "SAFTLIB           ", 19);
  
//   socket.attach(&eb_slave_sdb, &eb_slave_handler);
// }

// void Device::set_msi_buffer_capacity(size_t capacity)
// {
//   msis.set_capacity(capacity);
// }

// class MSI_Source : public saftbus::Source
// {
// 	public:
// 	bool prepare(std::chrono::milliseconds &timeout_ms);
// 	bool check();
// 	bool dispatch(sigc::slot_base* slot);
// };

// bool MSI_Source::prepare(std::chrono::milliseconds &timeout_ms)
// {
//   // returning true means immediately ready
//   bool result;
//   if (Device::msis.empty()) {
//     result = false;
//   } else {
//     timeout_ms = 0;
//     result = true;
//   }
//   return result;
// }

// bool MSI_Source::check()
// {
//   bool result = !Device::msis.empty(); // true means ready after glib's poll
//   return result;
// }

// bool MSI_Source::dispatch()
// {
//   // Don't process more than 10 MSIs in one go (must give dbus some service too)
//   int limit = 10;
  
//   // Process any pending MSIs
//   while (!Device::msis.empty() && --limit) {
//     Device::MSI msi = Device::msis.front();
//     Device::msis.pop_front();
    
//     Device::irqMap::iterator i = Device::irqs.find(msi.address);
//     if (i != Device::irqs.end()) {
//       try {
//         i->second(msi.data);
//       } catch (const etherbone::exception_t& ex) {
//         clog << kLogErr << "Unhandled etherbone exception in MSI handler for 0x" 
//              << std::hex << msi.address << ": " << ex << std::endl;
//       } catch (std::exception& ex) {
//         clog << kLogErr << "Unhandled std::exception exception in MSI handler for 0x" 
//              << std::hex << msi.address << ": " << ex.what() << std::endl;
//       } catch (...) {
//         clog << kLogErr << "Unhandled unknown exception in MSI handler for 0x" 
//              << std::hex << msi.address << std::endl;
//       }
//     } else {
//       clog << kLogErr << "No handler for MSI 0x" << std::hex << msi.address << std::endl;
//     }
//   }
  
//   return true;
// }

// sigc::connection Device::attach(const std::shared_ptr<Slib::MainLoop>& loop)
// {
// 	saftbus::Loop::get_default().connect<MSI_Source>()
// 	return out;
// }

} // namespace saftlib
