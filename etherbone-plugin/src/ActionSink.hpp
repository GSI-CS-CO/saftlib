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
#include <saftbus/service.hpp>

// @saftbus-include
#include <Time.hpp>

#include "Condition.hpp"

namespace eb_plugin {

class EcaDriver;

// de.gsi.eb_plugin.ActionSink:
/// @brief An output through which actions flow.
/// 
/// Conditions created on this ActionSink specify which timing events are
/// translated into actions. These actions have execution timestamps which
/// determine when the action is to be executed, precise to the nanosecond.
/// 
/// More specialized versions of this interface provide the 'NewCondition'
/// method to create conditions specific to the type of ActionSink.  For
/// example, SoftwareActionSinks create conditions that emit signals to
/// software.  This interface captures the functionality common to all
/// ActionSinks, such as atomic toggle, offset constraints, and
/// introspection. In particular, ActionSinks have common failure modes.
/// 
/// Actions are queued for delivery at the appropriate time, in hardware. 
/// Hardware has limited storage, reflected by the Fill, Capacity, and
/// MostFull properties.  These should be monitored to ensure that the
/// queue never overflows. 
/// 
/// The first failure mode of an ActionSink is that the queue overflows. 
/// In this case, the hardware drops an action.  Obviously, this is a
/// critical error which may result in an undefined state.  To prevent
/// these failures, MostFull should be kept below some safety margin with
/// respect to Capacity.  Note: several distinct ActionSinks may share
/// underlying hardware, and the Fill property is shared amongst all
/// instances.  Each overflow is recorded in the OverflowCount register. 
/// Due the rate at which this counter might increase, the API throttles
/// updates using the SignalRate property.
/// 
/// Another critical error for an ActionSink is the possibility of a
/// late action.  This indicates that hardware was instructed to execute an
/// action at a time in the past.  This is typically caused by either a
/// malfunctioning data master, desynchronized clocks, or conditions with
/// large negative offsets.  This is a critical failure as it might leave
/// the system in an undefined state.  Conditions may be configured to
/// either drop or execute late actions.  If late actions are dropped, a
/// magnet might never be turned off.  If late actions are executed, a
/// magnet might be turned on again after it was supposed to be turned off
/// (ie: the actions get misordered).  In any case, LateCount is increased.
/// 
/// Similar to late actions, one can also have early actions. If an action
/// is scheduled for execution too far into the future, the timing receiver
/// will choose to mark it as early. This prevents these actions from
/// permanently consuming space in the finite hardware buffers. Early
/// actions are also critical failures as it can leave the system in an
/// undefined state, just as a late action. Conditions may be configured to
/// either drop or execute early actions.
/// 
/// The final misordering failure for an ActionSink is the possibilty of a
/// Conflict.  If two actions are scheduled to be executed at the same
/// nanosecond, their relative order is undefined.  These conflicts are a
/// critical error as they may leave the system in an undefined state, just
/// as with early and late actions.  Conflicts should be prevented by never
/// creating two Conditions on the same ActionSink which could occur at the
/// same time.  Note that it is NOT a Conflict for two actions to be
/// executed at the same time by two different ActionSinks.  For software,
/// this means that two programs, each with their own SoftwareActionSink do
/// not need to be concerned about conflicts between their schedules.  As
/// another example, two different LEMO output cables (corresponding to two
/// OutputActionSinks) can be toggled high at the same time.
/// 
/// Finally, these is the possibility of a delayed action. Unlike late,
/// early, and conflicting actions, delayed actions are never misordered. 
/// Thus, delayed actions are typically not as severe a failure mode, and
/// Conditions default to allowing their execution.  Delays can happen when
/// the delivery rate of actions (potentially 1GHz) exceeds the capability
/// of the receiver to process the actions.  For example, an output might
/// require 100ns to deliver an action.  If two actions are scheduled for
/// delivery back-to-back with 1ns between, the second action is delayed. 
/// For SoftwareActionSinks, delays can probably always be ignored because
/// the handler is much much slower than the hardware.  For a kicker, on
/// the other hand, a delay is probably a critical error.
class ActionSink 
{
	public:
		/// @brief  ActionSink constructor
		///
		/// @param eca points to the EcaDriver object on which the ActionSink is created
		/// @param name name of the ActionSink
		/// @param channel the ECA channel that feeds the ActionSink
		/// @param num     subchannel of the ECA channel
		///
		/// Create a new action sink on an ECA deive with a given name. If the name is an 
		/// empty string, a name in the form "_<number>" will be generated. The ActionSink
		/// is attached to the given ECA channel/subchannel.
		ActionSink(EcaDriver* eca
		         , const std::string& name
		         , unsigned channel
		         , unsigned num
		         , saftbus::Container *container = nullptr);
		virtual ~ActionSink();
		
		/// @brief  Atomically toggle the active status of conditions.
		///
		/// When reconfiguring an ActionSink, it is sometimes necessary to apply
		/// many changes simultaneously.  To achieve this, simply use
		/// NewCondition to create all the new conditions in the inactive state. 
		/// Then use this method to simultaneous toggle all conditions on this
		/// ActionSink.  The new active conditions will be applied such that on
		/// one nanosecond, the old set is active and on the next nanosecond the
		/// new set is active.
		///
		// @saftbus-export
		void ToggleActive();

		/// @brief Report the number of currently pending actions.
		/// @return Number of pending actions.
		///
		/// The timing receiver hardware can only queue a limited number of
		/// actions.  This method reports the current number of queued actions,
		/// which includes actions from all users of the underlying hardware.  So
		/// two programs each using a SoftwareActionSink will potentially see an
		/// increase in this value when both programs are active.  Also, this
		/// value can change very rapidly and is thus changes are not signalled. 
		/// Polling it is likely to miss short fluctuations.  See MostFull for a
		/// better approach to monitoring.
		///
		// @saftbus-export
		uint16_t ReadFill();

		/// @brief  All conditions created on this ActionSink.
		/// @return All conditions created on this ActionSink.
		///
		/// All active and inactive conditions on the ActionSink.
		/// Each object path implements the the matching Condition interface;
		/// for example, a SoftwareActionSink will have SoftwareConditions.
		// @saftbus-export
		std::vector< std::string > getAllConditions() const;

		/// @brief  All active conditions created on this ActionSink.
		/// @return All active conditions created on this ActionSink.
		///
		/// All active conditions on the ActionSink.
		/// Each object path implements the the matching Condition interface;
		/// for example, a SoftwareActionSink will have SoftwareConditions.
		///
		// @saftbus-export
		std::vector< std::string > getActiveConditions() const;

		/// @brief  All inactive conditions created on this ActionSink.
		/// @return All inactive conditions created on this ActionSink.
		///
		/// All inactive conditions on the ActionSink.
		/// Each object path implements the the matching Condition interface;
		/// for example, a SoftwareActionSink will have SoftwareConditions.
		///
		// @saftbus-export
		std::vector< std::string > getInactiveConditions() const;

		/// @brief  Minimum allowed offset (nanoseconds) usable in NewCondition.
		/// @return Minimum allowed offset (nanoseconds) usable in NewCondition.
		///
		/// Large offsets are almost always an error. A very negative offset
		/// could result in Late actions.  By default, no condition may be
		/// created with an offset smaller than -100us.  Attempts to create
		/// conditions with offsets less than MinOffset result in an error. 
		/// Change this property to override this safety feature.
		///
		// @saftbus-export
		int64_t getMinOffset() const;
		// @saftbus-export
		void setMinOffset(int64_t val);
		
		/// @brief  Maximum allowed offset (nanoseconds) usable in NewCondition.
		/// @return Maximum allowed offset (nanoseconds) usable in NewCondition.
		///
		/// Large offsets are almost always an error. A large positive offset
		/// could result in early actions being created.  By default, no
		/// condition may have an offset larger than 1s.  Attempts to create
		/// conditions with offsets greater than MaxOffset result in an error. 
		/// Change this property to override this safety feature.
		///
		// @saftbus-export
		int64_t getMaxOffset() const;
		// @saftbus-export
		void setMaxOffset(int64_t val);

		/// @brief  Nanoseconds between event and earliest execution of an action.
		/// @return Nanoseconds between event and earliest execution of an action.
		///
		// @saftbus-export
		uint64_t getLatency() const;

		/// @brief  Actions further into the future than this are early.
		/// @return Actions further into the future than this are early.
		///
		/// If an action is scheduled for execution too far into the future, it
		/// gets truncated to at most EarlyThreshold nanoseconds into the future.
		///
		// @saftbus-export
		uint64_t getEarlyThreshold() const;

		/// @brief  The maximum number of actions queueable without Overflow.
		/// @return The maximum number of actions queueable without Overflow.
		///
		/// The timing receiver hardware can only queue a limited number of
		/// actions.  This property reports the maximum number of actions that
		/// may be simultaneously queued.  Be aware that this resource may be
		/// shared between multiple ActionSinks.  For example, all
		/// SoftwareActionSinks share a common underlying queue in hardware. 
		/// This Capacity represents the total size, which must be shared.
		///
		// @saftbus-export
		uint16_t getCapacity() const;

		/// @brief  Report the largest number of pending actions seen.
		/// @return Report the largest number of pending actions seen.
		///
		/// The timing receiver hardware can only queue a limited number of
		/// actions.  This property reports the highest Fill level seen by the
		/// hardware since it was last reset to 0.  Keep in mind that the queue
		/// may be shared, including actions from all users of the underlying
		/// hardware.  So two programs each using a SoftwareActionSink will
		/// potentially see an increase in this value when both programs are
		/// active.
		///
		// @saftbus-export
		uint16_t getMostFull() const;
		// @saftbus-export
		void setMostFull(uint16_t val);

		/// @brief  Minimum interval between updates (nanoseconds, default 100ms).
		/// @return Minimum interval between updates (nanoseconds, default 100ms).
		///
		/// The properties OverflowCount, ActionCount, LateCount, EarlyCount,
		/// ConflictCount, and DelayedCount can increase rapidly. To prevent
		/// excessive CPU load, SignalRate imposes a minimum cooldown between
		/// updates to these values.
		///
		// @saftbus-export
		std::chrono::nanoseconds getSignalRate() const;
		// @saftbus-export
		void setSignalRate(std::chrono::nanoseconds val);

		/// @brief The number of actions lost due to Overflow.
		///
		/// The underlying hardware queue may overflow once Fill=Capacity. This
		/// is a critical error condition that must be handlded.  The causes may
		/// be either: 1- the actions have an execution time far enough in the
		/// future that too many actions are buffered before they are executed,
		/// or 2- the receiving component is unable to execute actions as quickly
		/// as the timing system delivers them.  The second case is particularly
		/// likely for SoftwareActionSinks that attempt to listen to high
		/// frequency events.  Even though SoftwareActionSinks share a common queue, 
		/// Overflow is reported only to the ActionSink whose action was dropped.
		/// As overflows can occur very rapidly, OverflowCount may increase by
		/// more than 1 between emissions. There is a minimum delay of SignalRate
		/// nanoseconds between updates to this property.
		///
		// @saftbus-export
		uint64_t getOverflowCount() const;
		// @saftbus-export
		void setOverflowCount(uint64_t val);

		/// @brief  The number of actions processed by the Sink.
		/// @return The number of actions processed by the Sink.
		///
		/// As actions can be emitted very rapidly, ActionCount may increase by
		/// more than 1 between emissions. There is a minimum delay of SignalRate
		/// nanoseconds between updates to this property.		
		///
		// @saftbus-export
		uint64_t getActionCount() const;
		// @saftbus-export
		void setActionCount(uint64_t val);

		/// @brief  The number of actions delivered late.
		/// @return The number of actions delivered late.
		/// 
		/// As described in the interface overview, an action can be late due to
		/// a buggy data master, loss of clock synchronization, or very negative
		/// condition offsets.  This is a critical failure as it can result in
		/// misordering of executed actions.  Each such failure increases this
		/// counter.  
		/// 
		/// As late actions can occur very rapidly, LateCount may increase by
		/// more than 1 between emissions. There is a minimum delay of SignalRate
		/// nanoseconds between updates to this property.		
		///
		// @saftbus-export
		uint64_t getLateCount() const;
		// @saftbus-export
		void setLateCount(uint64_t val);
		/// @brief:      An example of a late action since last LateCount change.
		///
		/// @param count    The new value of LateCount when this signal was raised.
		/// @param event    The event identifier of the late action.
		/// @param param    The parameter field, whose meaning depends on the event ID.
		/// @param deadline The desired execution timestamp (event time + offset).
		/// @param executed The actual execution timestamp.
		/// Keep in mind that an action is only counted as late if it is
		/// scheduled for the past.  An action which leaves the timing receiver
		/// after its deadline, due to a slow consumer, is a delayed (not late)
		/// action.		
		///
		// @saftbus-signal		
		std::function< void(uint32_t count, uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed) > SigLate;



		///	@brief  The number of actions delivered early.
		///	@return The number of actions delivered early.
		///
		///	As described in the interface overview, an action can be early due to
		///	a buggy data master, loss of clock synchronization, or very positive
		///	condition offsets.  This is a critical failure as it can result in
		///	misordering of executed actions.  Each such failure increases this
		///	counter.  
		///
		///	As early acitons can occur very rapidly, EarlyCount may increase by
		///	more than 1 between emissions. There is a minimum delay of SignalRate
		///	nanoseconds between updates to this property.
		///
       	// @saftbus-export
		uint64_t getEarlyCount() const;
		// @saftbus-export
		void setEarlyCount(uint64_t val);
		/// @brief     An example of an early action since last EarlyCount change.
		///
		/// @param count    The new value of LateCount when this signal was raised.
		/// @param event    The event identifier of the early action.
		/// @param param    The parameter field, whose meaning depends on the event ID.
		/// @param deadline The desired execution timestamp (event time + offset).
		/// @param executed The actual execution timestamp.
		// @saftbus-signal		
		std::function< void(uint32_t count, uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed) > SigEarly;

		/// @brief  The number of actions which conflicted. 
		/// @return The number of actions which conflicted. 
		///
		/// If two actions should have been executed simultaneously by the same
		/// ActionSink, they are executed in an undefined order. Each time this
		/// happens, the ConflictCount is increased. 
		/// As conflicts can occur very rapidly, ConflictCount may increase by
		/// more than 1 between emissions. There is a minimum delay of SignalRate
		/// nanoseconds between updates to this property.
		///
		// @saftbus-export
		uint64_t getConflictCount() const;
		// @saftbus-export
		void setConflictCount(uint64_t val);
		/// @brief   An example of a conflict since last ConflictCount change.
		///
		/// @param count    The new value of ConflictCount when this signal was raised.
		/// @param event    The event identifier of a conflicting action.
		/// @param param    The parameter field, whose meaning depends on the event ID.
		/// @param deadline The scheduled action execution timestamp (event time + offset).
		/// @param executed The timestamp when the action was actually executed.	
		///
		// @saftbus-signal		
		std::function< void(uint64_t count, uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed) > SigConflict;

		/// @brief  The number of actions which have been delayed.
		/// @return The number of actions which have been delayed.
		///
		/// The timing receiver emits actions potentially every nanosecond. In
		/// the case that the receiver cannot immediately process an action, the
		/// timing receiver delays the action until the receiver is ready. This
		/// can happen either because the receiver was still busy with a previous
		/// action or the output was used externally (bus arbitration).
		/// As actions can be emitted very rapidly, DelayedCount may increase by
		/// more than 1 between emissions. There is a minimum delay of SignalRate
		/// nanoseconds between updates to this property.
		///
		// @saftbus-export
		uint64_t getDelayedCount() const;
		// @saftbus-export
		void setDelayedCount(uint64_t val);
		/// @brief   An example of a delayed action the last DelayedCount change.
		///
		/// @param count    The value of DelayedCount when this signal was raised.
		/// @param event    The event identifier of the delayed action.
		/// @param param    The parameter field, whose meaning depends on the event ID.
		/// @param deadline The desired execution timestamp (event time + offset).
		/// @param executed The timestamp when the action was actually executed.	
		///
		// @saftbus-signal		
		std::function< void(uint64_t count, uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed) > SigDelayed;
		

		// Do the grunt work to create a condition
		// typedef sigc::slot<std::shared_ptr<Condition>, const Condition::Condition_ConstructorType&> ConditionConstructor;
		// std::string NewConditionHelper(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, bool tagIsKey);//, ConditionConstructor constructor);

		/// @brief Sanity checks for the construction arguments of a condition and selection of a unique random number 
		unsigned prepareCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, bool tagIsKey);

		void compile();
		
		// The name under which this ActionSink is listed in TimingReceiver::Iterfaces
		const std::string &getObjectName() const { return name; }

		const std::string &getObjectPath() const { return object_path; }

		// Used by TimingReciever::compile
		typedef std::map< uint32_t, std::unique_ptr<Condition> > Conditions;
		const Conditions& getConditions() const { return conditions; }
		unsigned getChannel() const { return channel; }
		unsigned getNum() const { return num; }
		
		// Receive MSI from TimingReceiver
		virtual void receiveMSI(uint8_t code);



		Condition *getCondition(const std::string object_path);
		
		// Useful for Condition destroy methods
		void removeCondition(uint32_t number);
		
	protected:
		std::string object_path;
		EcaDriver* eca;
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
		Conditions conditions;
		

		saftbus::Container *container;
};

}

#endif
