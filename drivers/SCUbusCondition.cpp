#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "SCUbusCondition.h"
#include "ActionSink.h"

namespace saftlib {

SCUbusCondition::SCUbusCondition(const ConstructorType& args)
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

Glib::RefPtr<SCUbusCondition> SCUbusCondition::create(const ConstructorType& args)
{
  return RegisteredObject<SCUbusCondition>::create(args.objectPath, args);
}

}
