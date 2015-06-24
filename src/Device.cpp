#define ETHERBONE_THROWS 1

#include <stdlib.h>
#include <string.h>
#include "Device.h"

namespace saftlib {

Device::Device(etherbone::Device d)
 : etherbone::Device(d), low(0), high(0)
{
  std::vector<sdb_device> pcie;
  std::vector<sdb_device> vme;
  
  // !!! replace with general purpose interrupt claim
  
  d.sdb_find_by_identity(0x651, 0x8a670e73, pcie);
  d.sdb_find_by_identity(0x651, 0x9326AA75, vme);
  
  if (pcie.size() == 1) {
    low  = pcie[0].sdb_component.addr_first;
    high = pcie[0].sdb_component.addr_last;
  } else if (vme.size() == 1) {
    low  = vme[0].sdb_component.addr_first;
    high = vme[0].sdb_component.addr_last;
  }
}

eb_address_t Device::request_irq(const sigc::slot<void,eb_data_t>& slot)
{
  eb_address_t irq;
  
  if (low == high) {
    throw etherbone::exception_t("request_irq/no_irq", EB_FAIL);
  }
  
  int retry;
  for (retry = 1000; retry > 0; --retry) {
    irq = low + (rand() % (high-low));
    irq &= ~7;
    if (irqs.find(irq) == irqs.end()) break;
  }
  
  if (retry == 0) {
    throw etherbone::exception_t("request_irq/no_free", EB_FAIL);
  }
  
  irqs[irq] = slot;
  return irq;
}

void Device::release_irq(eb_address_t irq)
{
  irqs.erase(irq);
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
  Device::irqMap::iterator i = Device::irqs.find(address);
  if (i != Device::irqs.end()) {
    i->second(data);
  }
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
  everything.sdb_component.addr_first = 0x4000;
  everything.sdb_component.addr_last  = 0xffffffffULL;
  everything.sdb_component.product.vendor_id = 0x651;
  everything.sdb_component.product.device_id = 0xefaa70;
  everything.sdb_component.product.version   = 1;
  everything.sdb_component.product.date      = 0x20150225;
  memcpy(everything.sdb_component.product.name, "SAFTLIB           ", 19);
  
  socket.attach(&everything, &handler);
}

} // saftlib
