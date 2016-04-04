#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "MILbusActionSink.h"
#include "MILbusCondition.h"

namespace saftlib {

MILbusActionSink::MILbusActionSink(ConstructorType args)
 : ActionSink(args.dev, args.name, args.channel, 0)
{
}

const char *MILbusActionSink::getInterfaceName() const
{
  return "MILbusActionSink";
}

Glib::ustring MILbusActionSink::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint16 tag)
{
  return NewConditionHelper(active, id, mask, offset, tag, false,
    sigc::ptr_fun(&MILbusCondition::create));
}

void MILbusActionSink::InjectTag(guint16 tag)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

Glib::RefPtr<MILbusActionSink> MILbusActionSink::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<MILbusActionSink>::create(objectPath, args);
}

}
