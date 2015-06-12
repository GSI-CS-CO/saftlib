#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SoftwareActionSink.h"
#include "SoftwareCondition.h"

namespace saftlib {

SoftwareActionSink::SoftwareActionSink(ConstructorType args)
 : ActionSink(args.dev, args.channel, args.destroy)
{
}

const char *SoftwareActionSink::getInterfaceName() const
{
  return "SoftwareActionSink";
}

Glib::ustring SoftwareActionSink::NewCondition(bool active, guint64 first, guint64 last, gint64 offset, guint32 guards)
{
  ownerOnly();
  
  if (offset < minOffset || offset > maxOffset)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "offset is out of range; adjust {min,max}Offset?");
  
  // Make a space for it in the container 
  conditions.push_back(Glib::RefPtr<Condition>());
  sigc::slot<void> destroy = sigc::bind(sigc::mem_fun(this, &ActionSink::removeCondition), --conditions.end());
  
  std::ostringstream str;
  str.imbue(std::locale("C"));
  str << getObjectPath() << "/_" << ++cond_count;
  Glib::ustring path = str.str();
  
  Glib::RefPtr<Condition> condition;
  try {
    // !!! only these next two lines belong in SoftwareActionSink. Rest should be in ActionSink!
    SoftwareCondition::ConstructorType args = { this, channel, active, first, last, offset, guards, 0, destroy };
    condition = SoftwareCondition::create(path, args);
    condition->initOwner(getConnection(), getSender());
    conditions.back() = condition;
    if (active) compile();
  } catch (...) {
    destroy();
    throw;
  }
  
  notify(active, !active);
  return condition->getObjectPath();
}

Glib::RefPtr<SoftwareActionSink> SoftwareActionSink::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SoftwareActionSink>::create(objectPath, args);
}

}
