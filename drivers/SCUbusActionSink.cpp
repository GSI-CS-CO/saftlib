#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SCUbusActionSink.h"
#include "SCUbusCondition.h"
#include "TimingReceiver.h"

namespace saftlib {

SCUbusActionSink::SCUbusActionSink(ConstructorType args)
 : ActionSink(args.dev, args.channel), scubus(args.scubus)
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
  etherbone::Cycle cycle;
  cycle.open(dev->getDevice());
#define SCUB_SOFTWARE_TAG_LO 0x20
#define SCUB_SOFTWARE_TAG_HI 0x24
  cycle.write(scubus + SCUB_SOFTWARE_TAG_LO, EB_BIG_ENDIAN|EB_DATA16, tag & 0xFFFF);
  cycle.write(scubus + SCUB_SOFTWARE_TAG_HI, EB_BIG_ENDIAN|EB_DATA16, (tag >> 16) & 0xFFFF);
  cycle.close();
}

Glib::RefPtr<SCUbusActionSink> SCUbusActionSink::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SCUbusActionSink>::create(objectPath, args);
}

}
