#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SCUbusCondition.h"
#include "ActionSink.h"

namespace saftlib {

SCUbusCondition::SCUbusCondition(ConstructorType args)
 : Condition(args)
{
}

guint32 SCUbusCondition::getTag() const
{
  return tag;
}

void SCUbusCondition::setTag(guint32 val)
{
  ownerOnly();
  if (val == tag) return;
  guint32 old = tag;
  
  tag = val;
  try {
    if (active) sink->compile();
    Tag(tag);
  } catch (...) {
    tag = old;
    throw;
  }
}

Glib::RefPtr<SCUbusCondition> SCUbusCondition::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<SCUbusCondition>::create(objectPath, args);
}

}
