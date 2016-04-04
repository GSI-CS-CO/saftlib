#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "MILbusCondition.h"
#include "ActionSink.h"

namespace saftlib {

MILbusCondition::MILbusCondition(ConstructorType args)
 : Condition(args)
{
}

guint16 MILbusCondition::getTag() const
{
  return tag;
}

void MILbusCondition::setTag(guint16 val)
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

Glib::RefPtr<MILbusCondition> MILbusCondition::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<MILbusCondition>::create(objectPath, args);
}

}
