#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "MILbusCondition.h"

namespace saftlib {

MILbusCondition::MILbusCondition(ConstructorType args)
 : Condition(args)
{
}

guint16 MILbusCondition::getTag() const
{
  return getRawTag();
}

Glib::RefPtr<MILbusCondition> MILbusCondition::create(Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<MILbusCondition>::create(objectPath, args);
}

}
