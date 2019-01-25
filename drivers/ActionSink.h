/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#ifndef ACTION_SINK_H
#define ACTION_SINK_H

#include <map>
#include "interfaces/iActionSink.h"
#include "Condition.h"

namespace saftlib {

class TimingReceiver;

class ActionSink : public Owned, public iActionSink
{
  public:
    ActionSink(const Glib::ustring& objectPath, TimingReceiver* dev, const Glib::ustring& name, unsigned channel, unsigned num, sigc::slot<void> destroy = sigc::slot<void>());
    ~ActionSink();
    
    void ToggleActive();
    guint16 ReadFill();
    
    std::vector< Glib::ustring > getAllConditions() const;
    std::vector< Glib::ustring > getActiveConditions() const;
    std::vector< Glib::ustring > getInactiveConditions() const;
    gint64 getMinOffset() const;
    gint64 getMaxOffset() const;
    guint64 getLatency() const;
    guint64 getEarlyThreshold() const;
    guint16 getCapacity() const;
    guint16 getMostFull() const;
    guint64 getSignalRate() const;
    guint64 getOverflowCount() const;
    guint64 getActionCount() const;
    guint64 getLateCount() const;
    guint64 getEarlyCount() const;
    guint64 getConflictCount() const;
    guint64 getDelayedCount() const;
    
    void setMinOffset(gint64 val);
    void setMaxOffset(gint64 val);
    void setMostFull(guint16 val);
    void setSignalRate(guint64 val);
    void setOverflowCount(guint64 val);
    void setActionCount(guint64 val);
    void setLateCount(guint64 val);
    void setEarlyCount(guint64 val);
    void setConflictCount(guint64 val);
    void setDelayedCount(guint64 val);
    
    // These property signals are available from base classes
    //   sigc::signal< void, const std::vector< Glib::ustring >& > AllConditions;
    //   sigc::signal< void, const std::vector< Glib::ustring >& > ActiveConditions;
    //   sigc::signal< void, const std::vector< Glib::ustring >& > InactiveConditions;
    //   sigc::signal< void, gint64 > MinOffset;
    //   sigc::signal< void, gint64 > MaxOffset;
    //   sigc::signal< void, guint16 > MostFull;
    //   sigc::signal< void, guint64 > SignalRate;
    //   sigc::signal< void, guint64 > OverflowCount;
    //   sigc::signal< void, guint64 > ActionCount;
    //   sigc::signal< void, guint64 > LateCount;
    //   sigc::signal< void, guint64 > EarlyCount;
    //   sigc::signal< void, guint64 > ConflictCount;
    //   sigc::signal< void, guint64 > DelayedCount;
    // These signals are available from base classes
    //   sigc::signal< void , guint32 , guint64 , guint64 , guint64 , guint64 > Late;
    //   sigc::signal< void , guint32 , guint64 , guint64 , guint64 , guint64 > Early;
    //   sigc::signal< void , guint64 , guint64 , guint64 , guint64 , guint64 > Conflict;
    //   sigc::signal< void , guint64 , guint64 , guint64 , guint64 , guint64 > Delayed;

    // Do the grunt work to create a condition
    typedef sigc::slot<std::shared_ptr<Condition>, const Condition::Condition_ConstructorType&> ConditionConstructor;
    Glib::ustring NewConditionHelper(bool active, guint64 id, guint64 mask, gint64 offset, guint32 tag, bool tagIsKey, ConditionConstructor constructor);

    // Emit AllConditions, ActiveConditions, InactiveConditions
    void notify(bool active = true, bool inactive = true);
    void compile();
    
    // The name under which this ActionSink is listed in TimingReceiver::Iterfaces
    virtual const char *getInterfaceName() const = 0;
    const Glib::ustring &getObjectName() const { return name; }

    // Used by TimingReciever::compile
    typedef std::map< guint32, std::shared_ptr<Condition> > Conditions;
    const Conditions& getConditions() const { return conditions; }
    unsigned getChannel() const { return channel; }
    unsigned getNum() const { return num; }
    
    // Receive MSI from TimingReceiver
    virtual void receiveMSI(guint8 code);
    
  protected:
    TimingReceiver* dev;
    Glib::ustring name;
    unsigned channel;
    unsigned num;
    
    // User controlled values
    gint64 minOffset, maxOffset;
    guint64 signalRate;
    
    // cached counters
    guint64 overflowCount;
    guint64 actionCount;
    guint64 lateCount;
    guint64 earlyCount;
    guint64 conflictCount;
    guint64 delayedCount;
    
    // last update of counters (for throttled)
    guint64 overflowUpdate;
    guint64 actionUpdate;
    guint64 lateUpdate;
    guint64 earlyUpdate;
    guint64 conflictUpdate;
    guint64 delayedUpdate;
    
    // constant hardware values
    guint64 latency;
    guint64 earlyThreshold;
    guint16 capacity;
    
    // pending timeouts to refresh counters
    sigc::connection overflowPending;
    sigc::connection actionPending;
    sigc::connection latePending;
    sigc::connection earlyPending;
    sigc::connection conflictPending;
    sigc::connection delayedPending;
    
    struct Record {
      guint64 event;
      guint64 param;
      guint64 deadline;
      guint64 executed;
      guint64 count;
    };
    Record fetchError(guint8 code);
    
    bool updateOverflow(guint64 time);
    bool updateAction(guint64 time);
    bool updateLate(guint64 time);
    bool updateEarly(guint64 time);
    bool updateConflict(guint64 time);
    bool updateDelayed(guint64 time);
    
    // conditions must come after dev to ensure safe cleanup on ~Condition
    Conditions conditions;
    
    // Useful for Condition destroy methods
    void removeCondition(Conditions::iterator i);
};

}

#endif
