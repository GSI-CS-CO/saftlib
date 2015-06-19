#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SCUbusActionSink.h"
#include "SCUbusCondition.h"

namespace saftlib {

SCUbusActionSink::SCUbusActionSink(ConstructorType args)
 : ActionSink(args.dev, args.channel)
{
}

const char *SCUbusActionSink::getInterfaceName() const
{
  return "SCUbusActionSink";
}

Glib::ustring SCUbusActionSink::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards, guint32 tag)
{
  return NewConditionHelper(active, id, mask, offset, guards, tag,
    sigc::ptr_fun(&SCUbusCondition::create));
}

void SCUbusActionSink::InjectTag(guint32 tag)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

Glib::RefPtr<SCUbusActionSink> SCUbusActionSink::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SCUbusActionSink>::create(objectPath, args);
}

}
