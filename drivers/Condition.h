#ifndef CONDITION_H
#define CONDITION_H

#include "interfaces/iCondition.h"
#include "Owned.h"

namespace saftlib {

class ActionSink;

class Condition : public Owned, public iCondition
{
  public:
    // if the created with active=true, you must manually run compile() on TimingReceiver
    Condition(ActionSink* sink, int channel, bool active, guint64 first, guint64 last, guint64 offset, guint64 guards, guint32 tag, sigc::slot<void> destroy = sigc::slot<void>());
    
    // iCondition
    guint64 getFirst() const;
    guint64 getLast() const;
    gint64 getOffset() const;
    gint64 getGuards() const;
    bool getActive() const;
    void setActive(bool val);
    // sigc::signal< void, bool > Active;
    
    // used by TimingReceiver and ActionSink
    guint32 getRawTag() const { return tag; }
    int getRawChannel() const { return channel; }
    void setRawActive(bool val) { active = val; }
    
  protected:
    ActionSink* sink;
    int channel;
    bool active;
    guint64 first;
    guint64 last;
    guint64 offset;
    guint64 guards;
    guint32 tag;
};

}

#endif
