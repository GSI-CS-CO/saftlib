#define ETHERBONE_THROWS 1
// #define DEBUG_COMPRESS
// #define DEBUG_COMPILE 1

#include <list>
#include <algorithm>
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
      ECA* eca, guint64 first, guint64 last, gint64 offset, guint32 tag,
      const Glib::ustring& owner, bool sw, bool hw, int channel);
    
    void Disown();
    void Delete();
    
    void setSoftwareActive(const bool& val);
    void setHardwareActive(const bool& val);
    
  protected:
    ECA_Condition(
      ECA* eca, guint64 first, guint64 last, gint64 offset, guint32 tag,
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
    unsigned aq_channel;
    
    std::vector<guint64> tag2delay;
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
  ECA* e, guint64 first, guint64 last, gint64 offset, guint32 tag,
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
  ECA_Condition_Service::setSoftwareActive(sw);
  ECA_Condition_Service::setHardwareActive(hw);
  setChannel(channel);
}

ECA_Condition::~ECA_Condition()
{
  if (subscription) getConnection()->signal_unsubscribe(subscription);
}

Glib::RefPtr<ECA_Condition> ECA_Condition::create(
  ECA* eca, guint64 first, guint64 last, gint64 offset, guint32 tag,
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
    ECA* ptr = eca;
    // remove reference => self destruct
    ptr->conditions.erase(index);
    ptr->recompile(); // "this" might be deleted by this line (hence ptr)
  } else {
    throw Gio::DBus::Error(Gio::DBus::Error::ACCESS_DENIED, "You are not my owner");
  }
}

void ECA_Condition::setSoftwareActive(const bool& val)
{
  ECA_Condition_Service::setSoftwareActive(val);
  try {
    eca->recompile();
  } catch (...) {
    ECA_Condition_Service::setSoftwareActive(false);
    throw;
  }
}

void ECA_Condition::setHardwareActive(const bool& val)
{
  ECA_Condition_Service::setHardwareActive(val);
  try {
    eca->recompile();
  } catch (...) {
    ECA_Condition_Service::setHardwareActive(false);
    throw;
  }
}

void ECA_Condition::owner_quit_handler(
  const Glib::RefPtr<Gio::DBus::Connection>& /* connection */, 
  const Glib::ustring& /* sender */, const Glib::ustring& /* object_path */, 
  const Glib::ustring& /* interface_name */, const Glib::ustring& /* method_name */, 
  const Glib::VariantContainerBase& /* parameters */)
{
  ECA* ptr = eca;
  // remove reference => self destruct
  ptr->conditions.erase(index);
  try { // handlers may not throw
    ptr->recompile(); // "this" might be deleted by this line (hence ptr)
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA_Condition::owner_quit_handler: " << e << std::endl;
  } catch (const Gio::DBus::Error& e) {
    std::cerr << "ECA_Condition::owner_quit_handler: GIO" << std::endl;
  }
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
  cycle.write(aq + ECAQ_DROPPED, EB_DATA32, 0);
  // How much old Q junk to flush?
  cycle.read(aq + ECAQ_QUEUED, EB_DATA32, &queued);
  // Which channel does this sit on?
  cycle.read(aq + ECAQ_META, EB_DATA32, &id);
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
  aq_channel = (id >> 16) & 0xFF;
  
  std::vector<Glib::ustring> paths;
  for (unsigned c = 0; c < channels.size(); ++c) {
    channels[c] = ECA_Channel::create(device, base, c, this);
    paths.push_back(channels[c]->getObjectPath());
  }
  
  setName(eca_extract_name(name));
  setFrequency("125MHz");
  setChannels(paths);

  setHandlers(true, arrival_irq, overflow_irq);
  
  // sets Conditions
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
    
    for (unsigned c = 0; c < channels.size(); ++c)
      channels[c].reset();
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

struct ECA_Merge {
  gint32  channel;
  gint64  offset;
  guint64 first;
  guint64 last;
  guint32 tag;
  ECA_Merge(gint32 c, gint64 o, guint64 f, guint64 l, guint32 t)
   : channel(c), offset(o), first(f), last(l), tag(t) { }
};

bool operator < (const ECA_Merge& a, const ECA_Merge& b)
{
  if (a.channel < b.channel) return true;
  if (a.channel > b.channel) return false;
  if (a.offset < b.offset) return true;
  if (a.offset > b.offset) return false;
  if (a.first < b.first) return true;
  if (a.first > b.first) return false;
  return false;
}

struct ECA_OpenClose {
  guint64 key;
  bool    open;
  guint64 subkey;
  gint64  offset;
  gint32  channel;
  guint32 tag;
  ECA_OpenClose(guint64 k, bool x, guint64 s, guint64 o, gint32 c, guint32 t)
   : key(k), open(x), subkey(s), offset(o), channel(c), tag(t) { }
};

// Using this heuristic, perfect containment never duplicates walk records
bool operator < (const ECA_OpenClose& a, const ECA_OpenClose& b)
{
  if (a.key < b.key) return true;
  if (a.key > b.key) return false;
  if (!a.open && b.open) return true; // close first
  if (a.open && !b.open) return false;
  if (a.subkey > b.subkey) return true; // open largest first, close smallest last
  if (a.subkey < b.subkey) return false;
  if (a.offset < b.offset) return a.open; // open smallest first, close largest last
  if (a.offset > b.offset) return !a.open;
  if (a.channel < b.channel) return a.open;
  if (a.channel > b.channel) return !a.open;
  if (a.tag < b.tag) return a.open;
  if (a.tag > b.tag) return !a.open;
  return false;
}

struct SearchEntry {
  guint64 event;
  gint16  index;
  SearchEntry(guint64 e, gint16 i) : event(e), index(i) { }
};

struct WalkEntry {
  gint64  offset;
  guint32 tag;
  gint16  next;
  guint8  channel;
  WalkEntry(gint64 o, guint32 t, gint16 n, guint8 c) : offset(o), tag(t), next(n), channel(c) { }
};

void ECA::recompile()
{
  typedef std::map<guint64, int> Offsets;
  typedef std::vector<ECA_Merge> Merges;
  Offsets offsets;
  Merges merges;
  guint32 next_tag = 0;
  
  // Step one is to merge overlapping, but compatible, ranges
  for (ConditionSet::iterator i = conditions.begin(); i != conditions.end(); ++i) {
    guint64 first  = (*i)->getFirst();
    guint64 last   = (*i)->getLast();
    guint64 offset = (*i)->getOffset();
    gint32  channel= (*i)->getChannel();
    guint32 tag    = (*i)->getTag();
    
    // reject rules on software channel
    if (channel == aq_channel)
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Cannot create hardware conditions on the software channel");
    
    // reject idiocy
    if (first > last)
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "first must be <= last");
    
    if ((*i)->getSoftwareActive()) {
      // software tag is based on offset
      std::pair<Offsets::iterator,bool> result = 
        offsets.insert(std::pair<guint64, guint32>(offset, next_tag));
      guint32 aq_tag = result.first->second;
      if (result.second) ++next_tag; // tag now used
      
      merges.push_back(ECA_Merge(aq_channel, offset, first, last, aq_tag));
    }
    if ((*i)->getHardwareActive()) {
      merges.push_back(ECA_Merge(channel, offset, first, last, tag));
    }
  }
  
  // Sort it by the merge criteria
  std::sort(merges.begin(), merges.end());
  
  // Compress the conditions: merging overlaps and convert to id-space
  typedef std::vector<ECA_OpenClose> ID_Space;
  ID_Space id_space;
  
  unsigned i = 0, j;
  while (i < merges.size()) {
    // Merge overlapping/touching records
    while ((j=i+1) < merges.size() && 
           merges[i].channel == merges[j].channel &&
           merges[i].offset  == merges[j].offset  &&
           (merges[j].first   == 0                ||
            merges[i].last    >= merges[j].first-1)) {
#if DEBUG_COMPRESS
      std::cerr << "I: " << merges[i].first << " " << merges[i].last << " " << merges[i].offset << " " << merges[i].channel << " " << merges[i].tag << std::endl;
      std::cerr << "I: " << merges[j].first << " " << merges[j].last << " " << merges[j].offset << " " << merges[j].channel << " " << merges[j].tag << std::endl;
#endif
      // they overlap, so tags must match!
      if (merges[i].tag != merges[j].tag)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Conflicting tags for overlapping conditions (same channel, same offset, same event)");
      // merge!
      merges[j].first = merges[i].first;
      merges[j].last  = std::max(merges[j].last, merges[i].last);
      i = j;
    }
    // push combined record to open/close pass
#if DEBUG_COMPRESS
    std::cerr << "O: " << merges[i].first << " " << merges[i].last << " " << merges[i].offset << " " << merges[i].channel << " " << merges[i].tag << std::endl;
#endif
    id_space.push_back(ECA_OpenClose(merges[i].first,  true,  merges[i].last,  merges[i].offset, merges[i].channel, merges[i].tag));
    if (merges[i].last != G_MAXUINT64)
      id_space.push_back(ECA_OpenClose(merges[i].last+1, false, merges[i].first, merges[i].offset, merges[i].channel, merges[i].tag));
    i = j;
  }
  
  // Don't need this any more
  merges.clear();
  
  // Sort it by the open/close criteria
  std::sort(id_space.begin(), id_space.end());
  
  // Representation used in hardware
  typedef std::vector<SearchEntry> Search;
  typedef std::vector<WalkEntry> Walk;
  Search search;
  Walk walk;
  Walk reflow;
  gint16 next = -1;
  guint64 cursor = 0;
  int reflows = 0;
  
  // Special-case at zero: skip closes and push leading record
  if (id_space.empty() || id_space[0].key != 0)
    search.push_back(SearchEntry(0, next));
  
  // Walk the remaining records and transform them to hardware!
  i = 0;
  while (i < id_space.size()) {
    cursor = id_space[i].key;
    
    while (i < id_space.size() && cursor == id_space[i].key && !id_space[i].open) {
      while (walk[next].offset  != id_space[i].offset ||
             walk[next].tag     != id_space[i].tag    ||
             walk[next].channel != id_space[i].channel) {
        reflow.push_back(walk[next]);
        next = walk[next].next;
        if (next == -1)
          throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Wes promised this was impossible");
      }
      next = walk[next].next;
      ++i;
    }
    
    // restore reflow records
    for (j = reflow.size(); j > 0; --j) {
      walk.push_back(reflow[j-1]);
      walk.back().next = next;
      next = walk.size()-1;
      ++reflows;
    }
    reflow.clear();
    
    // push the opens
    while (i < id_space.size() && cursor == id_space[i].key && id_space[i].open) {
      // ... could try to find an existing tail to re-use here
      walk.push_back(WalkEntry(id_space[i].offset, id_space[i].tag, next, id_space[i].channel));
      next = walk.size()-1;
      ++i;
    }
    
    search.push_back(SearchEntry(cursor, next));
  }
  
#if DEBUG_COMPILE
  std::cerr << "Table compilation complete! Reflows necessary: " << reflows << "\n";
  for (i = 0; i < search.size(); ++i)
    std::cerr << "S: " << search[i].event << " " << search[i].index << "\n";
  for (i = 0; i < walk.size(); ++i)
    std::cerr << "W: " << walk[i].offset << " " << walk[i].tag << " " << walk[i].next << " " << (int)walk[i].channel << "\n";
  std::cerr << std::flush; 
#endif

  if (walk.size() > table_size || search.size() > table_size*2)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Too many conditions to fit in hardware");
  
  setFree(42);
  
  etherbone::Cycle cycle;
  for (unsigned i = 0; i < 2*table_size; ++i) {
    /* Duplicate last entry to fill out the table */
    const SearchEntry& se = (i<search.size())?search[i]:search.back();
    eb_data_t first = (se.index==-1)?0:(UINT32_C(0x80000000)|se.index);
    
    cycle.open(device);
    cycle.write(base + ECA_SEARCH, EB_DATA32, i);
    cycle.write(base + ECA_FIRST,  EB_DATA32, first);
    cycle.write(base + ECA_EVENT1, EB_DATA32, se.event >> 32);
    cycle.write(base + ECA_EVENT0, EB_DATA32, se.event & UINT32_C(0xFFFFFFFF));
    cycle.close();
  }
  
  for (unsigned i = 0; i < walk.size(); ++i) {
    const WalkEntry& we = walk[i];
    eb_data_t next = (we.next==-1)?0:(UINT32_C(0x80000000)|we.next);
    
    cycle.open(device);
    cycle.write(base + ECA_WALK,    EB_DATA32, i);
    cycle.write(base + ECA_NEXT,    EB_DATA32, next);
    cycle.write(base + ECA_DELAY1,  EB_DATA32, we.offset >> 32);
    cycle.write(base + ECA_DELAY0,  EB_DATA32, we.offset & UINT32_C(0xFFFFFFFF));
    cycle.write(base + ECA_TAG,     EB_DATA32, we.tag);
    cycle.write(base + ECA_CHANNEL, EB_DATA32, we.channel);
    cycle.close();
  }
  
  // Flip the tables
  device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_FLIP);
  
  // !!! fuck - not atomic WRT table flip
  tag2delay.resize(next_tag);
  for (Offsets::iterator o = offsets.begin(); o != offsets.end(); ++o)
    tag2delay[o->second] = o->first;
  
  std::vector<Glib::ustring> paths;
  for (ConditionSet::iterator i = conditions.begin(); i != conditions.end(); ++i)
    paths.push_back((*i)->getObjectPath());
  setConditions(paths);
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
    
    bool conflict = (flags & 2) != 0;
    bool late     = (flags & 2) != 0;
    
    /* Report error cases */
    if (late)     Late    (event, param, time, tef);
    if (conflict) Conflict(event, param, time, tef);
    
    for (ConditionSet::iterator i = conditions.begin(); i != conditions.end(); ++i) {
      // Software rules use tag to filter conditions intelligently
      if ((*i)->getFirst() <= event && event <= (*i)->getLast() && 
          (*i)->getOffset() == tag2delay[tag]) {
        (*i)->Action(event, param, time, tef, late, conflict);
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
  // !!! probe for real
  Glib::RefPtr<ECA> object = ECA::create(Directory::get()->devices()[0], 0x80, 0x7ffffff0, 0x40);
  Directory::get()->add("ECA", object->getObjectPath(), object);
}

static Driver<ECA_Probe> eca;

} // saftlib
