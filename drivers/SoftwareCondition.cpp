#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SoftwareCondition.h"

namespace saftlib {

SoftwareCondition::SoftwareCondition(ConstructorType args)
 : Condition(args)
{
}

Glib::RefPtr<SoftwareCondition> SoftwareCondition::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SoftwareCondition>::create(objectPath, args);
}

}
