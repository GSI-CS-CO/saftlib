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
    ActionSink(const std::string& objectPath, TimingReceiver* dev, const std::string& name, unsigned channel, unsigned num, sigc::slot<void> destroy = sigc::slot<void>());
    ~ActionSink();
    
    void ToggleActive();
    uint16_t ReadFill();
    
    std::vector< std::string > getAllConditions() const;
    std::vector< std::string > getActiveConditions() const;
    std::vector< std::string > getInactiveConditions() const;
    int64_t getMinOffset() const;
    int64_t getMaxOffset() const;
    uint64_t getLatency() const;
    uint64_t getEarlyThreshold() const;
    uint16_t getCapacity() const;
    uint16_t getMostFull() const;
    uint64_t getSignalRate() const;
    uint64_t getOverflowCount() const;
    uint64_t getActionCount() const;
    uint64_t getLateCount() const;
    uint64_t getEarlyCount() const;
    uint64_t getConflictCount() const;
    uint64_t getDelayedCount() const;
    
    void setMinOffset(int64_t val);
    void setMaxOffset(int64_t val);
    void setMostFull(uint16_t val);
    void setSignalRate(uint64_t val);
    void setOverflowCount(uint64_t val);
    void setActionCount(uint64_t val);
    void setLateCount(uint64_t val);
    void setEarlyCount(uint64_t val);
    void setConflictCount(uint64_t val);
    void setDelayedCount(uint64_t val);
    
    // Do the grunt work to create a condition
    typedef sigc::slot<std::shared_ptr<Condition>, const Condition::Condition_ConstructorType&> ConditionConstructor;
    std::string NewConditionHelper(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, bool tagIsKey, ConditionConstructor constructor);

    void compile();
    
    // The name under which this ActionSink is listed in TimingReceiver::Iterfaces
    virtual const char *getInterfaceName() const = 0;
    const std::string &getObjectName() const { return name; }

    // Used by TimingReciever::compile
    typedef std::map< uint32_t, std::shared_ptr<Condition> > Conditions;
    const Conditions& getConditions() const { return conditions; }
    unsigned getChannel() const { return channel; }
    unsigned getNum() const { return num; }
    
    // Receive MSI from TimingReceiver
    virtual void receiveMSI(uint8_t code);
    
  protected:
    TimingReceiver* dev;
    std::string name;
    unsigned channel;
    unsigned num;
    
    // User controlled values
    int64_t minOffset, maxOffset;
    uint64_t signalRate;
    
    // cached counters
    uint64_t overflowCount;
    uint64_t actionCount;
    uint64_t lateCount;
    uint64_t earlyCount;
    uint64_t conflictCount;
    uint64_t delayedCount;
    
    // last update of counters (for throttled)
    uint64_t overflowUpdate;
    uint64_t actionUpdate;
    uint64_t lateUpdate;
    uint64_t earlyUpdate;
    uint64_t conflictUpdate;
    uint64_t delayedUpdate;
    
    // constant hardware values
    uint64_t latency;
    uint64_t earlyThreshold;
    uint16_t capacity;
    
    // pending timeouts to refresh counters
    sigc::connection overflowPending;
    sigc::connection actionPending;
    sigc::connection latePending;
    sigc::connection earlyPending;
    sigc::connection conflictPending;
    sigc::connection delayedPending;
    
    struct Record {
      uint64_t event;
      uint64_t param;
      uint64_t deadline;
      uint64_t executed;
      uint64_t count;
    };
    Record fetchError(uint8_t code);
    
    bool updateOverflow(uint64_t time);
    bool updateAction(uint64_t time);
    bool updateLate(uint64_t time);
    bool updateEarly(uint64_t time);
    bool updateConflict(uint64_t time);
    bool updateDelayed(uint64_t time);
    
    // conditions must come after dev to ensure safe cleanup on ~Condition
    Conditions conditions;
    
    // Useful for Condition destroy methods
    void removeCondition(Conditions::iterator i);
};

}

#endif
