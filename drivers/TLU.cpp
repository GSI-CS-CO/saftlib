#define ETHERBONE_THROWS 1

#include <sstream>
#include <iostream>
#include "ObjectRegistry.h"
#include "Driver.h"
#include "TLU.h"
#include "TLU_Channel.h"
#include "tlu_regs.h"

#define OBJECT_PATH "/de/gsi/saftlib/TLU"

namespace saftlib {

class TLU_Channel : public RegisteredObject<TLU_Channel_Service>
{
  public:
    ~TLU_Channel();
    
    void setEnabled(const bool& val);
    void setLatchEdge(const bool& val);
    void setStableTime(const guint32& val);
    
    void GetTime(guint64& result);
    
    static Glib::RefPtr<TLU_Channel> create(saftlib::Device& device, eb_address_t base, int channel);
    
  protected:
    TLU_Channel(Device& device, eb_address_t base, int channel);
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

TLU_Channel::TLU_Channel(Device& device_, eb_address_t base_, int channel_)
 : RegisteredObject<TLU_Channel_Service>(path(device_, base_, channel_)),
   device(device_), base(base_), channel(channel_), 
   irq(device.request_irq(sigc::mem_fun(*this, &TLU_Channel::irq_handler)))
{
  setEnabled(false);
  setHandler(true, irq, 0xdeadbeef);
  setLatchEdge(true);
  setStableTime(8);
}

TLU_Channel::~TLU_Channel()
{
  setHandler(false);
  setEnabled(false);
  device.release_irq(irq);
}

Glib::RefPtr<TLU_Channel> TLU_Channel::create(Device& device, eb_address_t base, int channel)
{
  return Glib::RefPtr<TLU_Channel>(new TLU_Channel(device, base, channel));
}

void TLU_Channel::setHandler(bool enable, eb_data_t address, eb_data_t message)
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

void TLU_Channel::setEnabled(const bool& val)
{
  if (val) {
    device.write(base + SLAVE_ACT_SET, EB_DATA32, 1<<channel);
  } else {
    device.write(base + SLAVE_ACT_CLR, EB_DATA32, 1<<channel);
  }
  TLU_Channel_Service::setEnabled(val);
}

void TLU_Channel::setLatchEdge(const bool& val)
{
  if (val) {
    device.write(base + SLAVE_EDG_SET, EB_DATA32, 1<<channel);
  } else {
    device.write(base + SLAVE_EDG_CLR, EB_DATA32, 1<<channel);
  }
  TLU_Channel_Service::setLatchEdge(val);
}

void TLU_Channel::setStableTime(const guint32& val)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + SLAVE_CH_SEL_RW, EB_DATA32, channel);
  cycle.write(base + SLAVE_STABLE_RW, EB_DATA32, val);
  cycle.close();
  TLU_Channel_Service::setStableTime(val);
}

void TLU_Channel::GetTime(guint64& val)
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
}

void TLU_Channel::irq_handler(eb_data_t)
{
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
}

class TLU : public RegisteredObject<TLU_Service>
{
  public:
    TLU(Devices& devices);
    void GetTime(guint64& result);
    
  protected:
    std::vector<Glib::RefPtr<TLU_Channel> > channels;
    void setIRQEnable(bool on);
};

TLU::TLU(Devices& devices)
 : RegisteredObject<TLU_Service>(OBJECT_PATH)
{
  std::vector<Glib::ustring> channel_names;
  std::vector<struct sdb_device> sdbs;
  
  // Scan all devices
  for (Devices::iterator device = devices.begin(); device != devices.end(); ++device) {
    sdbs.clear();
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
        channels.push_back(TLU_Channel::create(*device, address, channel));
        channel_names.push_back(channels.back()->getObjectPath());
      }
      
      // Flush all FIFOs and enable interrupts
      device->write(address + SLAVE_CLEAR_SET, EB_DATA32, 0xffffffffU);
      device->write(address + SLAVE_IE_RW,     EB_DATA32, 1);
    }
  }
  
  setChannels(channel_names);
}

void TLU::GetTime(guint64& result)
{
  if (!channels.empty())
    channels.front()->GetTime(result);
}

static Driver<TLU> tlu;

} // saftlib
