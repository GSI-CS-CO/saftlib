#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "OutputCondition.h"
#include "ActionSink.h"

namespace saftlib {

OutputCondition::OutputCondition(ConstructorType args)
 : Condition(args)
{
}

bool OutputCondition::getOn() const
{
  return tag;
}

void OutputCondition::setOn(bool val)
{
  ownerOnly();
  if (val == tag) return;
  guint32 old = tag;
  
  tag = val;
  try {
    if (active) sink->compile();
    On(tag);
  } catch (...) {
    tag = old;
    throw;
  }
}

Glib::RefPtr<OutputCondition> OutputCondition::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<OutputCondition>::create(objectPath, args);
}

}
