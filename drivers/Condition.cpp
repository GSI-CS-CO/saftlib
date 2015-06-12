#define ETHERBONE_THROWS 1

#include "Condition.h"
#include "ActionSink.h"
#include "TimingReceiver.h"

namespace saftlib {

Condition::Condition(ActionSink* sink_, int channel_, bool active_, guint64 first_, guint64 last_, guint64 offset_, guint64 guards_, guint32 tag_, sigc::slot<void> destroy)
 : Owned(destroy), sink(sink_), channel(channel_), 
   active(active_), first(first_), last(last_), offset(offset_), guards(guards_), tag(tag_)
{
}

guint64 Condition::getFirst() const
{
  return first;
}

guint64 Condition::getLast() const
{
  return last;
}

gint64 Condition::getOffset() const
{
  return offset;
}

gint64 Condition::getGuards() const
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
