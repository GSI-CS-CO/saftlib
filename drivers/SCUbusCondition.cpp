#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SCUbusCondition.h"

namespace saftlib {

SCUbusCondition::SCUbusCondition(ConstructorType args)
 : Condition(args)
{
}

guint32 SCUbusCondition::getTag() const
{
  return getRawTag();
}

Glib::RefPtr<SCUbusCondition> SCUbusCondition::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SCUbusCondition>::create(objectPath, args);
}

}
