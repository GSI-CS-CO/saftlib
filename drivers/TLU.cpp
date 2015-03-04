#define ETHERBONE_THROWS 1

#include <sstream>
#include <iostream>
#include "RegisteredObject.h"
#include "Driver.h"
#include "interfaces/TLU.h"
#include "tlu_regs.h"

#define OBJECT_PATH "/de/gsi/saftlib/TLU"

namespace saftlib {

class TLU : public RegisteredObject<TLU_Service>
{
  public:
    ~TLU();
    
    void setEnabled(const bool& val);
    void setLatchEdge(const bool& val);
    void setStableTime(const guint32& val);
    
    void CurrentTime(guint64& result);
    
    static Glib::RefPtr<TLU> create(saftlib::Device& device, eb_address_t base, int channel);
    static void probe();
    
  protected:
    TLU(Device& device, eb_address_t base, int channel);
    void irq_handler(eb_data_t);
    void setHandler(bool enable, eb_data_t address = 0, eb_data_t message = 0);
    
    Device& device;
    eb_address_t base;
    int channel;
    eb_address_t irq;
};

static Glib::ustring path(Device& device_, eb_address_t base_, int channel_)
{
  std::ostringstream stream;
  stream << OBJECT_PATH "/" << device_.getName() << "_" << std::hex << base_ << "_" << channel_;
  return stream.str();
}

TLU::TLU(Device& device_, eb_address_t base_, int channel_)
 : RegisteredObject<TLU_Service>(path(device_, base_, channel_)),
   device(device_), base(base_), channel(channel_), 
   irq(device.request_irq(sigc::mem_fun(*this, &TLU::irq_handler)))
{
  // throw through
  setEnabled(false);
  setHandler(true, irq, 0xdeadbeef);
  setLatchEdge(true);
  setStableTime(8);
}

TLU::~TLU()
{
  try { // destructors cannot throw
    device.release_irq(irq);
    setHandler(false);
    setEnabled(false);
  } catch (const etherbone::exception_t& e) {
    std::cerr << "TLU::~TLU: " << e << std::endl;
  }
  std::cout << "TLU destroyed" << std::endl;
}

Glib::RefPtr<TLU> TLU::create(Device& device, eb_address_t base, int channel)
{
  return Glib::RefPtr<TLU>(new TLU(device, base, channel));
}

void TLU::setHandler(bool enable, eb_data_t address, eb_data_t message)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  
  if (enable) {
    cycle.write(base + SLAVE_MSK_SET, EB_DATA32, 1<<channel);
  } else {
    cycle.write(base + SLAVE_MSK_CLR, EB_DATA32, 1<<channel);
  }
  
  cycle.write(base + SLAVE_CH_SEL_RW,     EB_DATA32, channel);
  cycle.write(base + SLAVE_TS_MSG_RW,     EB_DATA32, message);
  cycle.write(base + SLAVE_TS_DST_ADR_RW, EB_DATA32, address);
  cycle.close();
}

void TLU::setEnabled(const bool& val)
{
  if (val) {
    device.write(base + SLAVE_ACT_SET, EB_DATA32, 1<<channel);
  } else {
    device.write(base + SLAVE_ACT_CLR, EB_DATA32, 1<<channel);
  }
  TLU_Service::setEnabled(val);
}

void TLU::setLatchEdge(const bool& val)
{
  if (val) {
    device.write(base + SLAVE_EDG_SET, EB_DATA32, 1<<channel);
  } else {
    device.write(base + SLAVE_EDG_CLR, EB_DATA32, 1<<channel);
  }
  TLU_Service::setLatchEdge(val);
}

void TLU::setStableTime(const guint32& val)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + SLAVE_CH_SEL_RW, EB_DATA32, channel);
  cycle.write(base + SLAVE_STABLE_RW, EB_DATA32, val);
  cycle.close();
  TLU_Service::setStableTime(val);
}

void TLU::CurrentTime(guint64& val)
{
  eb_data_t hi, lo;
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.read(base + SLAVE_TC_GET_0, EB_DATA32, &hi);
  cycle.read(base + SLAVE_TC_GET_1, EB_DATA32, &lo);
  cycle.close();
  
  val = hi;
  val <<= 32;
  val |= lo;
  val <<= 3;
}

void TLU::irq_handler(eb_data_t)
{
  try { // irq handlers cannot throw
    etherbone::Cycle cycle;
    eb_data_t hi, lo, sub;
    cycle.open(device);
    cycle.write(base + SLAVE_CH_SEL_RW,  EB_DATA32, channel);
    cycle.read (base + SLAVE_TS_GET_0,   EB_DATA32, &hi);
    cycle.read (base + SLAVE_TS_GET_1,   EB_DATA32, &lo);
    cycle.read (base + SLAVE_TS_GET_2,   EB_DATA32, &sub);
    cycle.write(base + SLAVE_TS_POP_OWR, EB_DATA32, 1);
    cycle.close();
    
    guint64 time;
    time = hi;
    time <<= 32;
    time |= lo;
    time <<= 3;
    time |= sub;
    
    Edge(time);
  } catch (const etherbone::exception_t& e) {
    std::cerr << "TLU::irq_handler: " << e << std::endl;
  }
}

void TLU::probe() 
{
  // Scan all devices
  Devices& devices = Directory::get()->devices();
  for (Devices::iterator device = devices.begin(); device != devices.end(); ++device) {
    std::vector<struct sdb_device> sdbs;
    device->sdb_find_by_identity(TLU_SLAVE_VENDOR_ID, TLU_SLAVE_DEVICE_ID, sdbs);
    // Scan all cores
    for (unsigned core = 0; core < sdbs.size(); ++core) {
      eb_address_t address = sdbs[core].sdb_component.addr_first;
      eb_data_t num_channels;
      
      // Disable interrupts and enumerate channels
      device->write(address + SLAVE_IE_RW, EB_DATA32, 0);
      device->read(address + SLAVE_CH_NUM_GET, EB_DATA32, &num_channels);
      
      // Create all dbus channel objects
      for (eb_data_t channel = 0; channel < num_channels; ++channel) {
        Glib::RefPtr<TLU> object = TLU::create(*device, address, channel);
        Directory::get()->add("TLU", object->getObjectPath(), object);
      }
      
      // Flush all FIFOs and enable interrupts
      device->write(address + SLAVE_CLEAR_SET, EB_DATA32, 0xffffffffU);
      device->write(address + SLAVE_IE_RW,     EB_DATA32, 1);
    }
  }
}

static Driver<TLU> tlu;

} // saftlib
