#define ETHERBONE_THROWS 1

#include <list>
#include <iostream>
#include <sstream>
#include "RegisteredObject.h"
#include "Driver.h"
#include "interfaces/ECA.h"
#include "interfaces/ECA_Channel.h"
#include "interfaces/ECA_Condition.h"
#include "eca_regs.h"

namespace saftlib {

class ECA_Condition;
class ECA_Channel;
class ECA;

typedef std::list<Glib::RefPtr<ECA_Condition> > ConditionSet;

class ECA_Condition : public RegisteredObject<ECA_Condition_Service>
{
  public:
    ~ECA_Condition();
    
    static Glib::RefPtr<ECA_Condition> create(
      ECA* eca, guint64 first, guint64 last, guint64 offset, guint32 tag,
      const Glib::ustring& owner, bool sw, bool hw, int channel);
    
    void Disown();
    void Delete();
    
    void setSoftwareActive(const bool& val);
    void setHardwareActive(const bool& val);
    
  protected:
    ECA_Condition(
      ECA* eca, guint64 first, guint64 last, guint64 offset, guint32 tag,
      const Glib::ustring& owner, bool sw, bool hw, int channel);
    void owner_quit_handler(const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&);

    ECA* eca;
    ConditionSet::iterator index;
    guint subscription;
};

class ECA_Channel : public RegisteredObject<ECA_Channel_Service>
{
  public:
    ~ECA_Channel();
    static Glib::RefPtr<ECA_Channel> create(Device& device, eb_address_t base, int channel, ECA* eca);
    
    void NewCondition(
      const guint64& first, const guint64& last, const gint64& offset, const guint32& tag,
      Glib::ustring& result);
    
    const guint32& getFill() const;
    const guint32& getMaxFill() const;
    const guint32& getActionCount() const;
    const guint32& getConflictCount() const;
    const guint32& getLateCount() const;
    void setMaxFill(const guint32& getMaxFill);
    void setActionCount(const guint32&);
    void setConflictCount(const guint32&);
    void setLateCount(const guint32&);
    
  protected:
    ECA_Channel(Device& device, eb_address_t base, int channel, ECA* eca);
    void irq_handler(eb_data_t);
    void setHandler(bool enable, eb_address_t irq = 0);
    
    Device& device;
    eb_address_t base;
    int channel;
    ECA* eca;
    eb_address_t irq;

  friend class ECA;
};

class ECA : public RegisteredObject<ECA_Service>
{
  public:
    ~ECA();
    
    static Glib::RefPtr<ECA> create(Device& device, eb_address_t base, eb_address_t stream, eb_address_t aq);
    
    void NewCondition(
      const guint64& first, const guint64& last, const gint64& offset,
      Glib::ustring& result);
    void InjectEvent(
      const guint64& event, const guint64& param, const guint64& time, const guint32& tef);
    void CurrentTime(guint64& result);
      
    void recompile();
    
    ConditionSet conditions;

  protected:
    ECA(Device& device, eb_address_t base, eb_address_t stream, eb_address_t aq);
    void overflow_handler(eb_data_t);
    void arrival_handler(eb_data_t);
    void setHandlers(bool enable, eb_address_t arrival = 0, eb_address_t overflow = 0);
    
    Device& device;
    eb_address_t base;
    eb_address_t stream;
    eb_address_t aq;
    eb_address_t overflow_irq;
    eb_address_t arrival_irq;
    unsigned table_size;
    
    std::vector<Glib::RefPtr<ECA_Channel> > channels;
};

static Glib::ustring condition_path(ECA* e, ECA_Condition* c)
{
  std::ostringstream str;
  str.imbue(std::locale("C"));
  str << e->getObjectPath() << "/cond_" << std::hex << (uintptr_t)c;
  return str.str();
}

ECA_Condition::ECA_Condition(
  ECA* e, guint64 first, guint64 last, guint64 offset, guint32 tag,
  const Glib::ustring& owner, bool sw, bool hw, int channel)
 : RegisteredObject<ECA_Condition_Service>(condition_path(e, this)), eca(e),
   subscription(
    getConnection()->signal_subscribe(
      sigc::mem_fun(this, &ECA_Condition::owner_quit_handler),
      "org.freedesktop.DBus",
      "org.freedesktop.DBus",
      "NameOwnerChanged",
      "/org/freedesktop/DBus",
      owner))
{
  setFirst(first);
  setLast(last);
  setOffset(offset);
  setTag(tag);
  setOwner(owner);
  setSoftwareActive(sw);
  setHardwareActive(hw);
  setChannel(channel);
}

ECA_Condition::~ECA_Condition()
{
  if (subscription) getConnection()->signal_unsubscribe(subscription);
  std::cerr << "Condition destructor" << std::endl;
}

Glib::RefPtr<ECA_Condition> ECA_Condition::create(
  ECA* eca, guint64 first, guint64 last, guint64 offset, guint32 tag,
  const Glib::ustring& owner, bool sw, bool hw, int channel)
{
  Glib::RefPtr<ECA_Condition> output(new ECA_Condition(
    eca, first, last, offset, tag, owner, sw, hw, channel));
  
  output->index = eca->conditions.insert(eca->conditions.begin(), output);
  try {
    eca->recompile(); // can throw if a conflict is found
  } catch (...) {
    eca->conditions.erase(output->index);
    throw;
  }
  
  return output;
}

void ECA_Condition::Disown()
{
  if (sender == getOwner()) {
    setOwner("");
    getConnection()->signal_unsubscribe(subscription);
    subscription = 0;
  } else {
    throw Gio::DBus::Error(Gio::DBus::Error::ACCESS_DENIED, "You are not my owner");
  }
}

void ECA_Condition::Delete()
{
  if (getOwner().empty() || getOwner() == sender) {
    // remove reference => self destruct
    eca->conditions.erase(index);
  } else {
    throw Gio::DBus::Error(Gio::DBus::Error::ACCESS_DENIED, "You are not my owner");
  }
}

void ECA_Condition::setSoftwareActive(const bool& val)
{
  ECA_Condition_Service::setSoftwareActive(val);
  eca->recompile();
}

void ECA_Condition::setHardwareActive(const bool& val)
{
  ECA_Condition_Service::setHardwareActive(val);
  eca->recompile();
}

void ECA_Condition::owner_quit_handler(
  const Glib::RefPtr<Gio::DBus::Connection>& /* connection */, 
  const Glib::ustring& /* sender */, const Glib::ustring& /* object_path */, 
  const Glib::ustring& /* interface_name */, const Glib::ustring& /* method_name */, 
  const Glib::VariantContainerBase& /* parameters */)
{
  Delete();
}

static Glib::ustring channel_path(ECA* e, int channel)
{
  std::ostringstream str;
  str.imbue(std::locale("C"));
  str << e->getObjectPath() << "/chan_" << std::hex << channel;
  return str.str();
}

static std::string eca_extract_name(eb_data_t* data) {
  char name8[64];
  
  /* Unshift/extract the name from the control register */
  int zero = -1;
  int nonzero = -1;
  for (unsigned j = 0; j < 64; ++j) {
    name8[j] = ((data[j] >> 16) & 0xFF);
    if (zero == -1 && nonzero == -1 && !name8[j])
      zero = j;
    if (zero != -1 && nonzero == -1 && name8[j])
      nonzero = j;
  }
  if (zero == -1) return std::string(&name8[0], 63); /* Not terminated => cannot unwrap */
  if (nonzero == -1) nonzero = 0; /* If was zeros till end, no shift needed */
  for (unsigned j = 0; j < 64; ++j) {
    name8[j] = ((data[(j+nonzero)&0x3f] >> 16) & 0xFF);
  }
    
  return std::string(&name8[0]);
}

ECA_Channel::ECA_Channel(Device& d, eb_address_t b, int c, ECA* e)
 : RegisteredObject<ECA_Channel_Service>(channel_path(e, c)), 
   device(d), base(b), channel(c), eca(e),
   irq(device.request_irq(sigc::mem_fun(*this, &ECA_Channel::irq_handler)))
{
  eb_data_t name[64];
  eb_data_t sizes;
  etherbone::Cycle cycle;
  
  cycle.open(device);
  // select
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  // grab info
  for (unsigned i = 0; i < 64; ++i)
    cycle.read(base + ECAC_CTL, EB_DATA32, &name[i]);
  cycle.read(base + ECA_INFO, EB_DATA32, &sizes);
  // enable (clear drain and freeze flags)
  cycle.write(base + ECAC_CTL, EB_DATA32, (ECAC_CTL_DRAIN|ECAC_CTL_FREEZE)<<8);
  cycle.close();
  
  setName(eca_extract_name(name));
  setSize(1 << ((sizes >> 16) & 0xff));
  setHandler(true, irq);
  
  // We are in charge now. Clean all old state out.
  setMaxFill(0);
  setActionCount(0);
  setConflictCount(0);
  setLateCount(0);
}

ECA_Channel::~ECA_Channel()
{
  try {
    device.release_irq(irq);
    setHandler(false);
    etherbone::Cycle cycle;
    
    // drain+freeze the channel
    cycle.open(device);
    cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
    cycle.write(base + ECAC_CTL, EB_DATA32, (ECAC_CTL_DRAIN|ECAC_CTL_FREEZE));
    cycle.close();
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA_Channel::~ECA_Channel: " << e << std::endl;
  }
}

Glib::RefPtr<ECA_Channel> ECA_Channel::create(Device& device, eb_address_t base, int channel, ECA* eca)
{
  return Glib::RefPtr<ECA_Channel>(new ECA_Channel(device, base, channel, eca));
}

void ECA_Channel::setHandler(bool enable, eb_address_t irq)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT,   EB_DATA32|EB_BIG_ENDIAN, channel << 16);
  cycle.write(base + ECAC_CTL,      EB_DATA32|EB_BIG_ENDIAN, ECAC_CTL_INT_MASK<<8);
  cycle.write(base + ECAC_INT_DEST, EB_DATA32|EB_BIG_ENDIAN, irq);
  if (enable)
    cycle.write(base + ECAC_CTL,    EB_DATA32|EB_BIG_ENDIAN, ECAC_CTL_INT_MASK);
  cycle.close();
}
            
void ECA_Channel::NewCondition(
  const guint64& first, const guint64& last, const gint64& offset, const guint32& tag,
  Glib::ustring& result)
{
  Glib::RefPtr<ECA_Condition> condition = ECA_Condition::create(
    eca, first, last, offset, tag, sender, false, true, channel);
  result = condition->getObjectPath();
}

const guint32& ECA_Channel::getFill() const
{
  eb_data_t data;
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  cycle.read (base + ECAC_FILL,   EB_DATA32, &data);
  cycle.close();
  const_cast<ECA_Channel*>(this)->ECA_Channel_Service::setFill((data >> 16) & 0xFFFFU);
  return ECA_Channel_Service::getFill();
}

const guint32& ECA_Channel::getMaxFill() const
{
  eb_data_t data;
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  cycle.read (base + ECAC_FILL,   EB_DATA32, &data);
  cycle.close();
  const_cast<ECA_Channel*>(this)->ECA_Channel_Service::setMaxFill(data & 0xFFFFU);
  return ECA_Channel_Service::getMaxFill();
}

void ECA_Channel::setMaxFill(const guint32& value)
{
  if (value > 0xFFFFU)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "setMaxFill: value too large");
  
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  cycle.write(base + ECAC_FILL,   EB_DATA32, value);
  cycle.close();
  ECA_Channel_Service::setMaxFill(value);
}

const guint32& ECA_Channel::getActionCount() const
{
  eb_data_t data;
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  cycle.read (base + ECAC_VALID,  EB_DATA32, &data);
  cycle.close();
  const_cast<ECA_Channel*>(this)->ECA_Channel_Service::setActionCount(data);
  return ECA_Channel_Service::getActionCount();
}

void ECA_Channel::setActionCount(const guint32& value)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  cycle.write(base + ECAC_VALID,  EB_DATA32, value);
  cycle.close();
  ECA_Channel_Service::setActionCount(value);
}

const guint32& ECA_Channel::getConflictCount() const
{
  eb_data_t data;
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT,   EB_DATA32, channel << 16);
  cycle.read (base + ECAC_CONFLICT, EB_DATA32, &data);
  cycle.close();
  const_cast<ECA_Channel*>(this)->ECA_Channel_Service::setConflictCount(data);
  return ECA_Channel_Service::getConflictCount();
}

void ECA_Channel::setConflictCount(const guint32& value)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT,   EB_DATA32, channel << 16);
  cycle.write(base + ECAC_CONFLICT, EB_DATA32, value);
  cycle.close();
  ECA_Channel_Service::setConflictCount(value);
}

const guint32& ECA_Channel::getLateCount() const
{
  eb_data_t data;
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  cycle.read (base + ECAC_LATE,   EB_DATA32, &data);
  cycle.close();
  const_cast<ECA_Channel*>(this)->ECA_Channel_Service::setLateCount(data);
  return ECA_Channel_Service::getLateCount();
}

void ECA_Channel::setLateCount(const guint32& value)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
  cycle.write(base + ECAC_LATE,   EB_DATA32, value);
  cycle.close();
  ECA_Channel_Service::setLateCount(value);
}

void ECA_Channel::irq_handler(eb_data_t)
{
  try { // interrupt handlers may not throw
    guint32 late_old, conflict_old;
    guint32 late_new, conflict_new;
    
    late_old = ECA_Channel_Service::getLateCount();
    late_new = getLateCount();
    if (late_old != late_new) Late();
    
    conflict_old = ECA_Channel_Service::getConflictCount();
    conflict_new = getConflictCount();
    if (conflict_old != conflict_new) Conflict();
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA_Channel::irq_handler: " << e << std::endl;
  }
}

static Glib::ustring eca_path(Device& d, eb_address_t a)
{
  std::ostringstream str;
  str.imbue(std::locale("C"));
  str << "/de/gsi/saftlib/ECA_" << d.getName() << "_" << std::hex << a;
  return str.str();
}

ECA::ECA(Device& d, eb_address_t b, eb_address_t s, eb_address_t a)
 : RegisteredObject<ECA_Service>(eca_path(d, b)),
   device(d), base(b), stream(s), aq(a),
   overflow_irq(device.request_irq(sigc::mem_fun(*this, &ECA::overflow_handler))),
   arrival_irq (device.request_irq(sigc::mem_fun(*this, &ECA::arrival_handler)))
{
  eb_data_t name[64];
  eb_data_t sizes, queued;
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
  cycle.write(aq + ECAQ_DROPPED, EB_DATA32, 0);
  // How much old Q junk to flush?
  cycle.read(aq + ECAQ_QUEUED, EB_DATA32, &queued);
  cycle.close();
  
  unsigned pop = 0;
  while (pop < queued) {
    unsigned batch = ((queued-pop)>64)?64:(queued-pop); // pop 64 at once
    cycle.open(device);
    for (unsigned i = 0; i < batch; ++i)
      cycle.write(aq + ECAQ_CTL, EB_DATA32, 1);
    cycle.close();
    pop += batch;
  }
  
  channels.resize((sizes >> 8) & 0xFF);
  table_size = 1 << ((sizes >> 24) & 0xFF);
  
  std::vector<Glib::ustring> paths;
  for (unsigned c = 0; c < channels.size(); ++c) {
    channels[c] = ECA_Channel::create(device, base, c, this);
    paths.push_back(channels[c]->getObjectPath());
  }
  
  setName(eca_extract_name(name));
  setFrequency("125MHz");
  setChannels(paths);

  setHandlers(true, arrival_irq, overflow_irq);
  
  // sets Free and Conditions
  recompile();
  
  // enable interrupts and actions
  device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_DISABLE<<8 | ECA_CTL_INT_ENABLE);
}

ECA::~ECA()
{
  try { // destructors may not throw
    device.release_irq(overflow_irq);
    device.release_irq(arrival_irq);
    setHandlers(false);
    
    // Disable interrupts and the ECA
    device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_INT_ENABLE<<8 | ECA_CTL_DISABLE);
    
    while (!conditions.empty())
      conditions.front()->Delete();
    
    for (unsigned c = 0; c < channels.size(); ++c) {
      channels[c].reset();
    }
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA::~ECA: " << e << std::endl;
  }
}

Glib::RefPtr<ECA> ECA::create(Device& d, eb_address_t b, eb_address_t s, eb_address_t a)
{
  return Glib::RefPtr<ECA>(new ECA(d, b, s, a));
}

void ECA::setHandlers(bool enable, eb_address_t arrival, eb_address_t overflow)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(aq + ECAQ_INT_MASK, EB_DATA32, 0);
  cycle.write(aq + ECAQ_ARRIVAL,  EB_DATA32, arrival);
  cycle.write(aq + ECAQ_OVERFLOW, EB_DATA32, overflow);
  if (enable) cycle.write(aq + ECAQ_INT_MASK, EB_DATA32, 3);
  cycle.close();
}

void ECA::NewCondition(
  const guint64& first, const guint64& last, const gint64& offset,
  Glib::ustring& result)
{
  Glib::RefPtr<ECA_Condition> condition = ECA_Condition::create(
    this, first, last, offset, 0, sender, true, false, -1);
  result = condition->getObjectPath();
}

void ECA::InjectEvent(
  const guint64& event, const guint64& param, const guint64& time, const guint32& tef)
{
  etherbone::Cycle cycle;
  
  cycle.open(device);
  cycle.write(stream, EB_DATA32, event >> 32);
  cycle.write(stream, EB_DATA32, event & 0xFFFFFFFFUL);
  cycle.write(stream, EB_DATA32, param >> 32);
  cycle.write(stream, EB_DATA32, param & 0xFFFFFFFFUL);
  cycle.write(stream, EB_DATA32, 0); // reserved
  cycle.write(stream, EB_DATA32, tef);
  cycle.write(stream, EB_DATA32, time >> 32);
  cycle.write(stream, EB_DATA32, time & 0xFFFFFFFFUL);
  cycle.close();
}

void ECA::CurrentTime(guint64& result)
{
  etherbone::Cycle cycle;
  eb_data_t time1, time0;
  cycle.open(device);
  cycle.read(base + ECA_TIME1, EB_DATA32, &time1);
  cycle.read(base + ECA_TIME0, EB_DATA32, &time0);
  cycle.close();
  
  result = time1;
  result <<= 32;
  result |= time0;
}

void ECA::recompile()
{
  unsigned free = 43;
  
  // !!! do something sensible
  
  std::vector<Glib::ustring> paths;
  for (ConditionSet::iterator i = conditions.begin(); i != conditions.end(); ++i) {
    paths.push_back((*i)->getObjectPath());
  }
  setConditions(paths);
  setFree(free);
}

void ECA::overflow_handler(eb_data_t)
{
  Overflow();
}

void ECA::arrival_handler(eb_data_t)
{
  try { // interrupt handlers may not throw
    etherbone::Cycle cycle;
    eb_data_t flags;
    eb_data_t event1, event0;
    eb_data_t param1, param0;
    eb_data_t tag,    tef;
    eb_data_t time1,  time0;
    
    cycle.open(device);
    cycle.read (aq + ECAQ_FLAGS,  EB_DATA32, &flags);
    cycle.read (aq + ECAQ_EVENT1, EB_DATA32, &event1);
    cycle.read (aq + ECAQ_EVENT0, EB_DATA32, &event0);
    cycle.read (aq + ECAQ_PARAM1, EB_DATA32, &param1);
    cycle.read (aq + ECAQ_PARAM0, EB_DATA32, &param0);
    cycle.read (aq + ECAQ_TAG,    EB_DATA32, &tag);
    cycle.read (aq + ECAQ_TEF,    EB_DATA32, &tef);
    cycle.read (aq + ECAQ_TIME1,  EB_DATA32, &time1);
    cycle.read (aq + ECAQ_TIME0,  EB_DATA32, &time0);
    cycle.write(aq + ECAQ_CTL,    EB_DATA32, 1); // pop
    cycle.close();
    
    guint64 event, param, time;
    event = event1; event <<= 32; event |= event0;
    param = event1; param <<= 32; param |= param0;
    time  = time1;  time  <<= 32; time  |= time0;
    
    // !!! do something with the flags
    
    for (ConditionSet::iterator i = conditions.begin(); i != conditions.end(); ++i) {
      if ((*i)->getFirst() <= event && event < (*i)->getLast()) {
        (*i)->Action(event, param, time, tef);
      }
    }
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA::arrival_handler: " << e << std::endl;
  }
}

class ECA_Probe
{
  public:
    static void probe();
    // start/stop maybe keep reference ... and add ctrl+c => cleanup
};

void ECA_Probe::probe()
{
  Glib::RefPtr<ECA> object = ECA::create(Directory::get()->devices()[0], 0x80, 0x7ffffff0, 0x40);
  Directory::get()->add("ECA", object->getObjectPath(), object);
}

static Driver<ECA_Probe> eca;

} // saftlib
