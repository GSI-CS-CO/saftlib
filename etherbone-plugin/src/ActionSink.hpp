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
#ifndef EB_PLUGIN_ACTION_SINK_HPP
#define EB_PLUGIN_ACTION_SINK_HPP

#include <map>
#include <chrono>
#include <vector>
#include <string>

#include <saftbus/loop.hpp>

namespace eb_plugin {

class TimingReceiver;

class ActionSink 
{
  public:
    ActionSink(const std::string& objectPath
             , TimingReceiver* dev
             , const std::string& name
             , unsigned channel
             , unsigned num
             );//, sigc::slot<void> destroy = sigc::slot<void>());
    ~ActionSink();
    
    // @saftbus-export
    void ToggleActive();
    // @saftbus-export
    uint16_t ReadFill();
    
    // @saftbus-export
    std::vector< std::string > getAllConditions() const;
    // @saftbus-export
    std::vector< std::string > getActiveConditions() const;
    // @saftbus-export
    std::vector< std::string > getInactiveConditions() const;
    // @saftbus-export
    int64_t getMinOffset() const;
    // @saftbus-export
    int64_t getMaxOffset() const;
    // @saftbus-export
    uint64_t getLatency() const;
    // @saftbus-export
    uint64_t getEarlyThreshold() const;
    // @saftbus-export
    uint16_t getCapacity() const;
    // @saftbus-export
    uint16_t getMostFull() const;
    // @saftbus-export
    std::chrono::nanoseconds getSignalRate() const;
    // @saftbus-export
    uint64_t getOverflowCount() const;
    // @saftbus-export
    uint64_t getActionCount() const;
    // @saftbus-export
    uint64_t getLateCount() const;
    // @saftbus-export
    uint64_t getEarlyCount() const;
    // @saftbus-export
    uint64_t getConflictCount() const;
    // @saftbus-export
    uint64_t getDelayedCount() const;
    
    // @saftbus-export
    void setMinOffset(int64_t val);
    // @saftbus-export
    void setMaxOffset(int64_t val);
    // @saftbus-export
    void setMostFull(uint16_t val);
    // @saftbus-export
    void setSignalRate(std::chrono::nanoseconds val);
    // @saftbus-export
    void setOverflowCount(uint64_t val);
    // @saftbus-export
    void setActionCount(uint64_t val);
    // @saftbus-export
    void setLateCount(uint64_t val);
    // @saftbus-export
    void setEarlyCount(uint64_t val);
    // @saftbus-export
    void setConflictCount(uint64_t val);
    // @saftbus-export
    void setDelayedCount(uint64_t val);
    
    // Do the grunt work to create a condition
    // typedef sigc::slot<std::shared_ptr<Condition>, const Condition::Condition_ConstructorType&> ConditionConstructor;
    // std::string NewConditionHelper(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, bool tagIsKey, ConditionConstructor constructor);

    void compile();
    
    // The name under which this ActionSink is listed in TimingReceiver::Iterfaces
    const std::string &getObjectName() const { return name; }

    const std::string &getObjectPath() const { return object_path; }

    // Used by TimingReciever::compile
    // typedef std::map< uint32_t, std::shared_ptr<Condition> > Conditions;
    // const Conditions& getConditions() const { return conditions; }
    unsigned getChannel() const { return channel; }
    unsigned getNum() const { return num; }
    
    // Receive MSI from TimingReceiver
    virtual void receiveMSI(uint8_t code);
    
  protected:
    std::string object_path;
    TimingReceiver* dev;
    std::string name;
    unsigned channel;
    unsigned num;

    
    // User controlled values
    int64_t minOffset, maxOffset;
    std::chrono::nanoseconds signalRate;
    
    // cached counters
    mutable uint64_t overflowCount;
    mutable uint64_t actionCount;
    mutable uint64_t lateCount;
    mutable uint64_t earlyCount;
    mutable uint64_t conflictCount;
    mutable uint64_t delayedCount;
    
    // last update of counters (for throttled)
    mutable std::chrono::steady_clock::time_point overflowUpdate;
    mutable std::chrono::steady_clock::time_point actionUpdate;
    mutable std::chrono::steady_clock::time_point lateUpdate;
    mutable std::chrono::steady_clock::time_point earlyUpdate;
    mutable std::chrono::steady_clock::time_point conflictUpdate;
    mutable std::chrono::steady_clock::time_point delayedUpdate;
    
    // constant hardware values
    uint64_t latency;
    uint64_t earlyThreshold;
    uint16_t capacity;
    
    // pending timeouts to refresh counters
    saftbus::Source* overflowPending;
    saftbus::Source* actionPending;
    saftbus::Source* latePending;
    saftbus::Source* earlyPending;
    saftbus::Source* conflictPending;
    saftbus::Source* delayedPending;
    
    struct Record {
      uint64_t event;
      uint64_t param;
      uint64_t deadline;
      uint64_t executed;
      uint64_t count;
    };
    Record fetchError(uint8_t code) const;
    
    bool updateOverflow() const;
    bool updateAction() const;
    bool updateLate() const;
    bool updateEarly() const;
    bool updateConflict() const;
    bool updateDelayed() const;
    
    // conditions must come after dev to ensure safe cleanup on ~Condition
    //Conditions conditions;
    
    // Useful for Condition destroy methods
    //void removeCondition(Conditions::iterator i);
};

}

#endif
