#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SoftwareCondition.h"

namespace saftlib {

SoftwareCondition::SoftwareCondition(ConstructorType args)
 : Condition(args.sink, args.channel, args.active, args.first, args.last, args.offset, args.guards, args.tag, args.destroy)
{
}

Glib::RefPtr<SoftwareCondition> SoftwareCondition::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SoftwareCondition>::create(objectPath, args);
}

}
