#ifndef ACTION_SINK_H
#define ACTION_SINK_H

#include "interfaces/iActionSink.h"
#include "Condition.h"

namespace saftlib {

class TimingReceiver;

class ActionSink : public Owned, public iActionSink
{
  public:
    ActionSink(TimingReceiver* dev, int channel, sigc::slot<void> destroy = sigc::slot<void>());
    
    void ToggleActive();
    std::vector< Glib::ustring > getAllConditions() const;
    std::vector< Glib::ustring > getActiveConditions() const;
    std::vector< Glib::ustring > getInactiveConditions() const;
    gint64 getMinOffset() const;
    gint64 getMaxOffset() const;
    guint32 getCapacity() const;
    guint32 getFill() const;
    guint32 getMostFull() const;
    guint32 getOverflowCount() const;
    guint32 getConflictCount() const;
    bool getExecuteLateActions() const;
    guint32 getLateCount() const;
    bool getGenerateDelayed() const;
    guint32 getDelayedCount() const;
    guint32 getActionCount() const;
    void setMinOffset(gint64 val);
    void setMaxOffset(gint64 val);
    void setMostFull(guint32 val);
    void setOverflowCount(guint32 val);
    void setConflictCount(guint32 val);
    void setExecuteLateActions(bool val);
    void setLateCount(guint32 val);
    void setGenerateDelayed(bool val);
    void setDelayedCount(guint32 val);
    void setActionCount(guint32 val);
    // These property signals are available from base classes
    //   sigc::signal< void, const std::vector< Glib::ustring >& > AllConditions;
    //   sigc::signal< void, const std::vector< Glib::ustring >& > ActiveConditions;
    //   sigc::signal< void, const std::vector< Glib::ustring >& > InactiveConditions;
    //   sigc::signal< void, gint64 > MinOffset;
    //   sigc::signal< void, gint64 > MaxOffset;
    //   sigc::signal< void, guint32 > MostFull;
    //   sigc::signal< void, bool > ExecuteLateActions;
    //   sigc::signal< void, bool > GenerateDelayed;
    // These signals are available from base classes
    //   sigc::signal< void > Overflow;
    //   sigc::signal< void , guint64 , guint64 , guint64 , guint64 , guint64 , guint64 > Conflict;
    //   sigc::signal< void , guint64 , guint64 , guint64 , guint64 > Late;
    //   sigc::signal< void , guint64 , guint64 , guint64 , guint64 > Delayed;
  
    // Emit AllConditions, ActiveConditions, InactiveConditions
    void notify(bool active = true, bool inactive = true);
    void compile();
    
    // Useful for Condition destroy methods
    void removeCondition(std::list< Glib::RefPtr<Condition> >::iterator i);

  protected:
    TimingReceiver* dev;
    int channel;
    int cond_count;
    gint64 minOffset, maxOffset;
    bool executeLateActions;
    bool generateDelayed;
    
    std::list< Glib::RefPtr<Condition> > conditions;
};

}

#endif
