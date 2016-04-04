#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SCUbusActionSink.h"
#include "SCUbusCondition.h"
#include "TimingReceiver.h"

namespace saftlib {

SCUbusActionSink::SCUbusActionSink(const ConstructorType& args)
 : ActionSink(args.objectPath, args.dev, args.name, args.channel, 0), scubus(args.scubus)
{
}

const char *SCUbusActionSink::getInterfaceName() const
{
  return "SCUbusActionSink";
}

Glib::ustring SCUbusActionSink::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 tag)
{
  return NewConditionHelper(active, id, mask, offset, tag, false,
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

Glib::RefPtr<SCUbusActionSink> SCUbusActionSink::create(const ConstructorType& args)
{
  return RegisteredObject<SCUbusActionSink>::create(args.objectPath, args);
}

}
