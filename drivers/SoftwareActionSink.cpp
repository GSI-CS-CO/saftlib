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

Glib::ustring SoftwareActionSink::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards)
{
  return NewConditionHelper(active, id, mask, offset, guards, 0,
    sigc::ptr_fun(&SoftwareCondition::create));
}

Glib::RefPtr<SoftwareActionSink> SoftwareActionSink::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SoftwareActionSink>::create(objectPath, args);
}

}
