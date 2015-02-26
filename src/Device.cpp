#include <stdlib.h>
#include <string.h>
#include "Device.h"

namespace saftlib {

Device::Device(etherbone::Device d, eb_address_t low_, eb_address_t high_)
 : etherbone::Device(d), low(low_), high(high_), name("pex0")
{
}

// Must use IRQs that are globally unique
typedef std::map<eb_address_t, sigc::slot<void, eb_data_t> > irq_map;
static irq_map irqs;

eb_address_t Device::request_irq(const sigc::slot<void,eb_data_t>& slot)
{
  eb_address_t irq;
  
  do {
    irq = low + (rand() % (high-low));
    irq &= ~7;
  } while (irqs.find(irq) != irqs.end());
  
  irqs[irq] = slot;
  return irq;
}

void Device::release_irq(eb_address_t irq)
{
  irqs.erase(irq);
}

static struct IRQ_Handler : public etherbone::Handler
{
  eb_status_t read (eb_address_t address, eb_width_t width, eb_data_t* data);
  eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);
} handler;

eb_status_t IRQ_Handler::read(eb_address_t address, eb_width_t width, eb_data_t* data)
{
  *data = 0;
  return EB_OK;
}

eb_status_t IRQ_Handler::write(eb_address_t address, eb_width_t width, eb_data_t data)
{
  irq_map::iterator i = irqs.find(address);
  if (i != irqs.end()) {
    i->second(data);
  }
  return EB_OK;
}

struct sdb_device everything;
void Device::hook_it_all(etherbone::Socket socket)
{
  everything.abi_class     = 0;
  everything.abi_ver_major = 0;
  everything.abi_ver_minor = 0;
  everything.bus_specific  = SDB_WISHBONE_WIDTH;
  everything.sdb_component.addr_first = 0x4000;
  everything.sdb_component.addr_last  = EB_ADDR_C(0xffffffff);
  everything.sdb_component.product.vendor_id = 0x651;
  everything.sdb_component.product.device_id = 0xefaa70;
  everything.sdb_component.product.version   = 1;
  everything.sdb_component.product.date      = 0x20150225;
  memcpy(everything.sdb_component.product.name, "SAFTLIB           ", 19);
  
  socket.attach(&everything, &handler);
}

} // saftlib
