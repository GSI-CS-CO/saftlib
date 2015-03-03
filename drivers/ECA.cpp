#define ETHERBONE_THROWS 1

#include <list>
#include <iostream>
#include <sstream>
#include "ObjectRegistry.h"
#include "Driver.h"
#include "ECA.h"
#include "ECA_Channel.h"
#include "ECA_Condition.h"
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

    ECA* eca;
    ConditionSet::iterator index;
};

class ECA_Channel : public RegisteredObject<ECA_Channel_Service>
{
  public:
    ~ECA_Channel();
    static Glib::RefPtr<ECA_Channel> create(Device& device, ECA* eca, int channel);
    
    void NewCondition(
      const guint64& first, const guint64& last, const gint64& offset, const guint32& tag,
      Glib::ustring& result);
    void Reset();
    
    const guint32& getFill() const;
    const guint32& getMaxFill() const;
    const guint32& getActionCount() const;
    const guint32& getConflictCount() const;
    const guint32& getLateCount() const;
    
  protected:
    ECA_Channel(Device& device, ECA* eca, int channel);
    void irq_handler(eb_data_t);
    void setHandler(bool enable, eb_address_t irq = 0);
    
    Device& device;
    ECA* eca;
    int channel;
    eb_address_t irq;
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
      
    void recompile();
    
    ConditionSet conditions;

  protected:
    ECA(Device& device, eb_address_t base, eb_address_t stream, eb_address_t aq);
    void overflow_handler(eb_data_t);
    void arrival_handler(eb_data_t);
    void name_owner_changed_handler(const Glib::ustring&, const Glib::ustring&, const Glib::ustring&);
    
    Device& device;
    eb_address_t base;
    eb_address_t stream;
    eb_address_t aq;
    eb_address_t overflow_irq;
    eb_address_t arrival_irq;
};

static Glib::ustring condition_path(ECA* e, ECA_Condition* c)
{
  std::ostringstream str;
  str << e->getObjectPath() << "/cond-" << std::hex << c;
  return str.str();
}

ECA_Condition::ECA_Condition(
  ECA* e, guint64 first, guint64 last, guint64 offset, guint32 tag,
  const Glib::ustring& owner, bool sw, bool hw, int channel)
 : RegisteredObject<ECA_Condition_Service>(condition_path(e, this)), eca(e)
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
  std::cerr << "Condition destructor" << std::endl;
}

Glib::RefPtr<ECA_Condition> ECA_Condition::create(
  ECA* eca, guint64 first, guint64 last, guint64 offset, guint32 tag,
  const Glib::ustring& owner, bool sw, bool hw, int channel)
{
  Glib::RefPtr<ECA_Condition> output(new ECA_Condition(
    eca, first, last, offset, tag, owner, sw, hw, channel));
  
  try {
    output->index = eca->conditions.insert(eca->conditions.begin(), output);
    eca->recompile(); // can throw if a conflict is found
  } catch (...) {
    output->unregister_self();
    eca->conditions.erase(output->index);
    throw;
  }
  
  return output;
}

void ECA_Condition::Disown()
{
  if (getSender() == getOwner()) {
    setOwner("");
  } else {
    throw Gio::DBus::Error(Gio::DBus::Error::ACCESS_DENIED, "You are not my owner");
  }
}

void ECA_Condition::Delete()
{
  if (getOwner().empty() || getOwner() == getSender()) {
    // remove references => self destruct
    unregister_self();
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

static Glib::ustring channel_path(ECA* e, int channel)
{
  std::ostringstream str;
  str << e->getObjectPath() << "/chan-" << std::hex << channel;
  return str.str();
}

ECA_Channel::ECA_Channel(Device& d, ECA* e, int c)
 : RegisteredObject<ECA_Channel_Service>(channel_path(e, c)), 
   device(d), eca(e), channel(c), 
   irq(device.request_irq(sigc::mem_fun(*this, &ECA_Channel::irq_handler)))
{
  // !!!
  setName("...");
  setSize(33);
  setHandler(true, irq);
}

ECA_Channel::~ECA_Channel()
{
  try {
    setHandler(false);
    device.release_irq(irq);
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA_Channel::~ECA_Channel: " << e << std::endl;
  }
}

Glib::RefPtr<ECA_Channel> ECA_Channel::create(Device& device, ECA* eca, int channel)
{
  return Glib::RefPtr<ECA_Channel>(new ECA_Channel(device, eca, channel));
}
            
void ECA_Channel::NewCondition(
  const guint64& first, const guint64& last, const gint64& offset, const guint32& tag,
  Glib::ustring& result)
{
  Glib::RefPtr<ECA_Condition> condition = ECA_Condition::create(
    eca, first, last, offset, tag, getSender(), false, true, channel);
  result = condition->getObjectPath();
}

void ECA_Channel::Reset()
{
  // !!!
}

const guint32& ECA_Channel::getFill() const
{
  // !!!
  return ECA_Channel_Service::getFill();
}

const guint32& ECA_Channel::getMaxFill() const
{
  // !!!
  return ECA_Channel_Service::getMaxFill();
}

const guint32& ECA_Channel::getActionCount() const
{
  // !!!
  return ECA_Channel_Service::getActionCount();
}

const guint32& ECA_Channel::getConflictCount() const
{
  // !!!
  return ECA_Channel_Service::getConflictCount();
}

const guint32& ECA_Channel::getLateCount() const
{
  // !!!
  return ECA_Channel_Service::getLateCount();
}

void ECA_Channel::irq_handler(eb_data_t)
{
  // ... decide which
  Conflict();
  Late();
}

void ECA_Channel::setHandler(bool enable, eb_address_t irq)
{
  // !!!
}

ECA::ECA(Device& d, eb_address_t b, eb_address_t s, eb_address_t a)
 : RegisteredObject<ECA_Service>("..."),
   device(d), base(b), stream(s), aq(a),
   overflow_irq(device.request_irq(sigc::mem_fun(*this, &ECA::overflow_handler))),
   arrival_irq (device.request_irq(sigc::mem_fun(*this, &ECA::arrival_handler)))
{
  // !!! hook name change
  // !!! setName setFrequency setChannels
  recompile();
}

ECA::~ECA()
{
  device.release_irq(overflow_irq);
  device.release_irq(arrival_irq);
}

void ECA::NewCondition(
  const guint64& first, const guint64& last, const gint64& offset,
  Glib::ustring& result)
{
  Glib::RefPtr<ECA_Condition> condition = ECA_Condition::create(
    this, first, last, offset, 0, getSender(), true, false, -1);
  result = condition->getObjectPath();
}

void ECA::InjectEvent(
  const guint64& event, const guint64& param, const guint64& time, const guint32& tef)
{
  // !!!
}

void ECA::recompile()
{
  setFree(22);
  // setConditions(...); ... based on conditions
}

void ECA::overflow_handler(eb_data_t)
{
  Overflow();
}

void ECA::arrival_handler(eb_data_t)
{
  // !!! walk conditions looking for matching rule
}

void ECA::name_owner_changed_handler(const Glib::ustring&, const Glib::ustring&, const Glib::ustring&)
{
  // !!! walk conditions looking for name
}

class ECA_Probe
{
  public:
    ECA_Probe(Devices& devices);
};

ECA_Probe::ECA_Probe(Devices& devices)
{
  // !!!
}

static Driver<ECA_Probe> eca;

} // saftlib
