#define ETHERBONE_THROWS 1

#include "Condition.h"
#include "ActionSink.h"
#include "TimingReceiver.h"

namespace saftlib {

Condition::Condition(Condition_ConstructorType args)
 : Owned(args.destroy), sink(args.sink), channel(args.channel), 
   active(args.active), id(args.id), mask(args.mask), offset(args.offset), guards(args.guards), tag(args.tag)
{
}

guint64 Condition::getID() const
{
  return id;
}

guint64 Condition::getMask() const
{
  return mask;
}

gint64 Condition::getOffset() const
{
  return offset;
}

guint64 Condition::getGuards() const
{
  return guards;
}

bool Condition::getActive() const
{
  return active;
}

void Condition::setActive(bool val)
{
  ownerOnly();
  if (val == active) return;
  
  active = val;
  try {
    sink->compile();
    Active(active);
    sink->notify(true, true);
  } catch (...) {
    active = !val;
  }
}

}
