#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SoftwareCondition.h"

namespace saftlib {

SoftwareCondition::SoftwareCondition(const ConstructorType& args)
 : Condition(args)
{
}

Glib::RefPtr<SoftwareCondition> SoftwareCondition::create(const ConstructorType& args)
{
  return RegisteredObject<SoftwareCondition>::create(args.objectPath, args);
}

}
