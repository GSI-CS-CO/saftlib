#define ETHERBONE_THROWS 1

#include <sstream>
#include <iostream>
#include <algorithm>

#include "RegisteredObject.h"
#include "Driver.h"
#include "TimingReceiver.h"
#include "eca_regs.h"

namespace saftlib {

TimingReceiver::TimingReceiver(ConstructorType args)
 : device(args.device),
   name(args.name),
   etherbonePath(args.etherbonePath),
   base(args.base),
   stream(args.stream),
   queue(args.queue),
   sas_count(0),
   overflow_irq(device.request_irq(sigc::mem_fun(*this, &TimingReceiver::overflow_handler))),
   arrival_irq (device.request_irq(sigc::mem_fun(*this, &TimingReceiver::arrival_handler)))
{
  eb_data_t name[64];
  eb_data_t sizes, queued, id;
  etherbone::Cycle cycle;
  
  cycle.open(device);
  // probe
  for (unsigned i = 0; i < 64; ++i)
    cycle.read(base + ECA_CTL, EB_DATA32, &name[i]);
  cycle.read(base + ECA_INFO, EB_DATA32, &sizes);
  // disable ECA actions and interrupts
  cycle.write(base + ECA_CTL, EB_DATA32, ECA_CTL_INT_ENABLE<<8 | ECA_CTL_DISABLE);
  cycle.close();
  
  cycle.open(device);
  // Clear old counters
  cycle.write(queue + ECAQ_DROPPED, EB_DATA32, 0);
  // How much old Q junk to flush?
  cycle.read(queue + ECAQ_QUEUED, EB_DATA32, &queued);
  // Which channel does this sit on?
  cycle.read(queue + ECAQ_META, EB_DATA32, &id);
  cycle.close();
  
  unsigned pop = 0;
  while (pop < queued) {
    unsigned batch = ((queued-pop)>64)?64:(queued-pop); // pop 64 at once
    cycle.open(device);
    for (unsigned i = 0; i < batch; ++i)
      cycle.write(queue + ECAQ_CTL, EB_DATA32, 1);
    cycle.close();
    pop += batch;
  }
  
  //channels = (sizes >> 8) & 0xFF;
  table_size = 1 << ((sizes >> 24) & 0xFF);
  queue_size = 1 << ((sizes >> 16) & 0xFF);
  aq_channel = (id >> 16) & 0xFF;
  
  setHandlers(true, arrival_irq, overflow_irq);
  
  // enable interrupts and actions
  device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_DISABLE<<8 | ECA_CTL_INT_ENABLE);
}

TimingReceiver::~TimingReceiver()
{
  try { // destructors may not throw
    device.release_irq(overflow_irq);
    device.release_irq(arrival_irq);
    setHandlers(false);
    
    // Disable interrupts and the ECA
    device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_INT_ENABLE<<8 | ECA_CTL_DISABLE);
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA::~ECA: " << e << std::endl;
  }
}

void TimingReceiver::setHandlers(bool enable, eb_address_t arrival, eb_address_t overflow)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(queue + ECAQ_INT_MASK, EB_DATA32, 0);
  cycle.write(queue + ECAQ_ARRIVAL,  EB_DATA32, arrival);
  cycle.write(queue + ECAQ_OVERFLOW, EB_DATA32, overflow);
  if (enable) cycle.write(queue + ECAQ_INT_MASK, EB_DATA32, 3);
  cycle.close();
}

void TimingReceiver::Remove()
{
  SAFTd::get()->RemoveDevice(name);
}

Glib::ustring TimingReceiver::getEtherbonePath() const
{
  return etherbonePath;
}

Glib::ustring TimingReceiver::getName() const
{
  return name;
}

void TimingReceiver::do_remove(Glib::ustring name)
{
  actionSinks.erase(name);
  SoftwareActionSinks(getSoftwareActionSinks());
}

static inline bool not_isalnum_(char c)
{
  return !(isalnum(c) || c == '_');
}

Glib::ustring TimingReceiver::NewSoftwareActionSink(const Glib::ustring& name_)
{
  Glib::ustring name(name_);
  if (name == "") {
    std::ostringstream str;
    str.imbue(std::locale("C"));
    str << "_" << ++sas_count;
    name = str.str();
  } else {
    if (actionSinks.find(name) != actionSinks.end())
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Name already in use");
    if (name[0] == '_')
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; leading _ is reserved");
    if (find_if(name.begin(), name.end(), not_isalnum_) != name.end())
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
  }
  
  // nest the object under our own name
  Glib::ustring path = getObjectPath() + "/" + name;
  
  sigc::slot<void> destroy = sigc::bind(sigc::mem_fun(this, &TimingReceiver::do_remove), name);
  
  SoftwareActionSink::ConstructorType args = { this, aq_channel, destroy };
  Glib::RefPtr<ActionSink> softwareActionSink = SoftwareActionSink::create(path, args);
  softwareActionSink->initOwner(getConnection(), getSender());
  
  actionSinks[name] = softwareActionSink;
  SoftwareActionSinks(getSoftwareActionSinks());
  Interfaces(getInterfaces());
  
  return path;
}

void TimingReceiver::InjectEvent(guint64 event, guint64 param, guint64 time)
{
  etherbone::Cycle cycle;
  
  cycle.open(device);
  cycle.write(stream, EB_DATA32, event >> 32);
  cycle.write(stream, EB_DATA32, event & 0xFFFFFFFFUL);
  cycle.write(stream, EB_DATA32, param >> 32);
  cycle.write(stream, EB_DATA32, param & 0xFFFFFFFFUL);
  cycle.write(stream, EB_DATA32, 0); // reserved
  cycle.write(stream, EB_DATA32, 0); // TEF
  cycle.write(stream, EB_DATA32, time >> 32);
  cycle.write(stream, EB_DATA32, time & 0xFFFFFFFFUL);
  cycle.close();
}

guint64 TimingReceiver::getCurrentTime() const
{
  etherbone::Cycle cycle;
  eb_data_t time1, time0;
  cycle.open(device);
  cycle.read(base + ECA_TIME1, EB_DATA32, &time1);
  cycle.read(base + ECA_TIME0, EB_DATA32, &time0);
  cycle.close();
  
  guint64 result;
  result = time1;
  result <<= 32;
  result |= time0;
  return result;
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getSoftwareActionSinks() const
{
  typedef std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator iterator;
  std::map< Glib::ustring, Glib::ustring > out;
  for (iterator i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    Glib::RefPtr<SoftwareActionSink> softwareActionSink =
      Glib::RefPtr<SoftwareActionSink>::cast_dynamic(i->second);
    if (!softwareActionSink) continue;
    out[i->first] = softwareActionSink->getObjectPath();
  }
  return out;
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getOutputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInoutputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::vector< Glib::ustring > TimingReceiver::getGuards() const
{
  std::vector< Glib::ustring > out;
  return out; // !!! not for cryring
}

std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > TimingReceiver::getInterfaces() const
{
  std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > out;
  typedef std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator iterator;
  for (iterator i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    out[i->second->getInterfaceName()][i->first] = i->second->getObjectPath();
  }
  return out;
}

guint32 TimingReceiver::getFree() const
{
  return table_size; // !!!
}

void TimingReceiver::overflow_handler(eb_data_t)
{
}

void TimingReceiver::arrival_handler(eb_data_t)
{
  try { // interrupt handlers may not throw
    etherbone::Cycle cycle;
    eb_data_t flags;
    eb_data_t event1, event0;
    eb_data_t param1, param0;
    eb_data_t tag,    tef;
    eb_data_t time1,  time0;
    
    cycle.open(device);
    cycle.read (queue + ECAQ_FLAGS,  EB_DATA32, &flags);
    cycle.read (queue + ECAQ_EVENT1, EB_DATA32, &event1);
    cycle.read (queue + ECAQ_EVENT0, EB_DATA32, &event0);
    cycle.read (queue + ECAQ_PARAM1, EB_DATA32, &param1);
    cycle.read (queue + ECAQ_PARAM0, EB_DATA32, &param0);
    cycle.read (queue + ECAQ_TAG,    EB_DATA32, &tag);
    cycle.read (queue + ECAQ_TEF,    EB_DATA32, &tef);
    cycle.read (queue + ECAQ_TIME1,  EB_DATA32, &time1);
    cycle.read (queue + ECAQ_TIME0,  EB_DATA32, &time0);
    cycle.write(queue + ECAQ_CTL,    EB_DATA32, 1); // pop
    cycle.close();
    
    guint64 event, param, time;
    event = event1; event <<= 32; event |= event0;
    param = event1; param <<= 32; param |= param0;
    time  = time1;  time  <<= 32; time  |= time0;
    
    bool conflict = (flags & 2) != 0;
    bool late     = (flags & 2) != 0;
    
    // do something with this !!!
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA::arrival_handler: " << e << std::endl;
  }
}

void TimingReceiver::compile()
{
  // !!!
}

void TimingReceiver::probe(OpenDevice& od)
{
  // !!! check board ID
  // !!! probe for real
  TimingReceiver::ConstructorType args = { od.device, od.name, od.etherbonePath, 0x80, 0x7ffffff0, 0x40 };
  od.ref = RegisteredObject<TimingReceiver>::create(od.objectPath, args);
}

static Driver<TimingReceiver> timingReceiver;

}
