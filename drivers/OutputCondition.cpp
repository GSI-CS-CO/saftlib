#define ETHERBONE_THROWS 1

#include "RegisteredObject.h"
#include "OutputCondition.h"
#include "ActionSink.h"

namespace saftlib {

OutputCondition::OutputCondition(const ConstructorType& args)
 : Condition(args)
{
}

bool OutputCondition::getOn() const
{
  return tag == 1;
}

void OutputCondition::setOn(bool v)
{
  guint32 val = v?1:2; // 1 = turn-on, 2 = turn-off
  
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

Glib::RefPtr<OutputCondition> OutputCondition::create(const ConstructorType& args)
{
  return RegisteredObject<OutputCondition>::create(args.objectPath, args);
}

}
