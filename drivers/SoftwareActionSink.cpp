#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SoftwareActionSink.h"

namespace saftlib {

SoftwareActionSink::SoftwareActionSink(ConstructorType args)
 : ActionSink(args.dev, args.channel, args.destroy)
{
}

Glib::ustring SoftwareActionSink::NewCondition(bool active, guint64 first, guint64 last, gint64 offset, guint32 guards)
{
  // !!!
}

Glib::RefPtr<SoftwareActionSink> SoftwareActionSink::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SoftwareActionSink>::create(objectPath, args);
}

}
