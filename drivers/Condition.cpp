#define ETHERBONE_THROWS 1

#include "Condition.h"
#include "ActionSink.h"
#include "TimingReceiver.h"

namespace saftlib {

Condition::Condition(ActionSink* sink_, int channel_, bool active_, guint64 id_, guint64 mask_, guint64 offset_, guint64 guards_, guint32 tag_, sigc::slot<void> destroy)
 : Owned(destroy), sink(sink_), channel(channel_), 
   active(active_), id(id_), mask(mask_), offset(offset_), guards(guards_), tag(tag_)
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
