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
    ~ActionSink();
    
    void ToggleActive();
    guint32 ReadOverflowCount();
    guint32 ReadConflictCount();
    guint32 ReadLateCount();
    guint32 ReadDelayedCount();
    guint32 ReadActionCount();
    void ResetOverflowCount();
    void ResetConflictCount();
    void ResetLateCount();
    void ResetDelayedCount();
    void ResetActionCount();
    std::vector< Glib::ustring > getAllConditions() const;
    std::vector< Glib::ustring > getActiveConditions() const;
    std::vector< Glib::ustring > getInactiveConditions() const;
    gint64 getMinOffset() const;
    gint64 getMaxOffset() const;
    guint32 getCapacity() const;
    guint32 getFill() const;
    guint32 getMostFull() const;
    bool getExecuteLateActions() const;
    bool getGenerateDelayed() const;
    void setMinOffset(gint64 val);
    void setMaxOffset(gint64 val);
    void setMostFull(guint32 val);
    void setExecuteLateActions(bool val);
    void setGenerateDelayed(bool val);
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

    // Do the grunt work to create a condition
    typedef sigc::slot<Glib::RefPtr<Condition>, const Glib::ustring&, Condition::Condition_ConstructorType> ConditionConstructor;
    Glib::ustring NewConditionHelper(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards, guint32 tag, ConditionConstructor constructor);

    // Emit AllConditions, ActiveConditions, InactiveConditions
    void notify(bool active = true, bool inactive = true);
    void compile();
    
    // Useful for Condition destroy methods
    void removeCondition(std::list< Glib::RefPtr<Condition> >::iterator i);
    
    // The name under which this ActionSink is listed in TimingReceiver::Iterfaces
    virtual const char *getInterfaceName() const = 0;

    // Used by TimingReciever::compile
    const std::list< Glib::RefPtr<Condition> >& getConditions() const { return conditions; }
    int getChannel() const { return channel; }
    
  protected:
    TimingReceiver* dev;
    int channel;
    int cond_count;
    gint64 minOffset, maxOffset;
    bool executeLateActions;
    bool generateDelayed;
    // conditions must come after dev to ensure safe cleanup on ~Condition
    std::list< Glib::RefPtr<Condition> > conditions;
};

}

#endif
