#ifndef CONDITION_H
#define CONDITION_H

#include "interfaces/iCondition.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;

class Condition : public Owned, public iCondition
{
  public:
    // if the created with active=true, you must manually run compile() on TimingReceiver
    Condition(TimingReceiver* dev, bool active, guint64 first, guint64 last, guint64 offset, guint64 guards, guint32 tag, sigc::slot<void> destroy = sigc::slot<void>());
    ~Condition();
    
    // iCondition
    guint64 getFirst() const;
    guint64 getLast() const;
    gint64 getOffset() const;
    gint64 getGuards() const;
    bool getActive() const;
    void setActive(bool val);
    // sigc::signal< void, bool > Active;
    
    // used by TimingReceiver
    guint32 getRawTag() const { return tag; }
    
  protected:
     TimingReceiver* dev;
     bool active;
     guint64 first;
     guint64 last;
     guint64 offset;
     guint64 guards;
     guint32 tag;
};

}

#endif
