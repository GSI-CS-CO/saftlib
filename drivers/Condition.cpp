#define ETHERBONE_THROWS 1

#include "Condition.h"
#include "TimingReceiver.h"

namespace saftlib {

Condition::Condition(TimingReceiver* dev_, bool active_, guint64 first_, guint64 last_, guint64 offset_, guint64 guards_, guint32 tag_, sigc::slot<void> destroy)
 : Owned(destroy),
   dev(dev_), active(active_), first(first_), last(last_), offset(offset_), guards(guards_), tag(tag_)
{
}

Condition::~Condition()
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
    dev->compile();
    Active(active);
  } catch (...) {
    active = !val;
  }
}

}
