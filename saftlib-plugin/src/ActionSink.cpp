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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include "ActionSink.hpp"
#include "ECA.hpp"
#include "eca_regs.h"
#include "eca_flags.h"
//#include "clog.h"

#include <saftbus/error.hpp>

#include <sstream>
#include <cassert>

namespace eb_plugin {

// ActionSink::ActionSink(const std::string& objectPath, TimingReceiver* dev_, const std::string& name_, unsigned channel_, unsigned num_, saftbus::Container *container_)//, sigc::slot<void> destroy)
ActionSink::ActionSink(ECA &eca_
                 	   , const std::string& action_sink_object_path
                     , const std::string& name_
                     , unsigned channel_
                     , unsigned num_
                     , saftbus::Container *container_)
 :	Owned(container_), 
   object_path(action_sink_object_path), 
	eca(eca_), name(name_), channel(channel_), num(num_),
	minOffset(-1000000000L),  maxOffset(1000000000L), signalRate(std::chrono::nanoseconds(100000000L)),
	overflowCount(0), actionCount(0), lateCount(0), earlyCount(0), conflictCount(0), delayedCount(0),
	container(container_)
{

	overflowUpdate = actionUpdate = lateUpdate = earlyUpdate = conflictUpdate = delayedUpdate = std::chrono::steady_clock::now();
	eb_data_t raw_latency, raw_offset_bits, raw_capacity, null;

	etherbone::Cycle cycle;
	cycle.open(eca.get_device());
	// Grab configuration
	cycle.write(eca.get_base_address() + ECA_CHANNEL_SELECT_RW,     EB_DATA32, channel);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_NUM_SELECT_RW, EB_DATA32, num);
	cycle.read (eca.get_base_address() + ECA_LATENCY_GET,           EB_DATA32, &raw_latency);
	cycle.read (eca.get_base_address() + ECA_OFFSET_BITS_GET,       EB_DATA32, &raw_offset_bits);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_CAPACITY_GET,  EB_DATA32, &raw_capacity);
	// Wipe out any old state:
	cycle.read (eca.get_base_address() + ECA_CHANNEL_OVERFLOW_COUNT_GET, EB_DATA32, &null);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_VALID_COUNT_GET,    EB_DATA32, &null);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_LATE);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_EARLY);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_CONFLICT);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_DELAYED);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
	cycle.close();

	latency = raw_latency;   // in nanoseconds
	earlyThreshold = 1;      // in nanoseconds
	earlyThreshold <<= raw_offset_bits;
	capacity = raw_capacity; // in actions
}

ActionSink::~ActionSink()
{
	std::cerr << "~ActionSink " << getObjectPath() << std::endl;
	// unhook any pending updates
	saftbus::Loop::get_default().remove(overflowPending);
	saftbus::Loop::get_default().remove(actionPending);
	saftbus::Loop::get_default().remove(latePending);
	saftbus::Loop::get_default().remove(earlyPending);
	saftbus::Loop::get_default().remove(conflictPending);
	saftbus::Loop::get_default().remove(delayedPending);

	if (container) {
		while (conditions.size()) {
			container->remove_object(conditions.begin()->second->getObjectPath());
		}
		assert(conditions.size() == 0);
	} else {
		std::cerr << "~ActionSink clear all conditions" << std::endl;
		conditions.clear();
	}

	std::cerr << "~ActionSink done " << std::endl;
	// No need to recompile; done in TimingReceiver.cpp
}

void ActionSink::ToggleActive()
{
	ownerOnly();

	// Toggle raw to prevent recompile at each step
	Conditions::iterator i;
	for (i = conditions.begin(); i != conditions.end(); ++i) {
		i->second->setRawActive(!i->second->getActive());
	}

	try {
		compile();
	} catch (...) {
		// failed => undo flips
		for (i = conditions.begin(); i != conditions.end(); ++i) {
			i->second->setRawActive(!i->second->getActive());
		}
		throw;
	}
}

uint16_t ActionSink::ReadFill()
{
	return eca.updateMostFull(channel);
}

std::vector< std::string > ActionSink::getAllConditions() const
{
	std::vector< std::string > out;
	Conditions::const_iterator i;
	for (i = conditions.begin(); i != conditions.end(); ++i) {
		out.push_back(i->second->getObjectPath());
	}
	return out;
}

std::vector< std::string > ActionSink::getActiveConditions() const
{
	std::vector< std::string > out;
	Conditions::const_iterator i;
	for (i = conditions.begin(); i != conditions.end(); ++i) {
		if (i->second->getActive()) out.push_back(i->second->getObjectPath());
	}
	return out;
}

std::vector< std::string > ActionSink::getInactiveConditions() const
{
	std::vector< std::string > out;
	Conditions::const_iterator i;
	for (i = conditions.begin(); i != conditions.end(); ++i) {
		if (!i->second->getActive()) out.push_back(i->second->getObjectPath());
	}
	return out;
}

int64_t ActionSink::getMinOffset() const
{
	return minOffset;
}

int64_t ActionSink::getMaxOffset() const
{
	return maxOffset;
}

uint64_t ActionSink::getEarlyThreshold() const
{
	return earlyThreshold;
}

uint64_t ActionSink::getLatency() const
{
	return latency;
}

uint16_t ActionSink::getCapacity() const
{
	return capacity;
}

uint16_t ActionSink::getMostFull() const
{
	return eca.getMostFull(channel);
}

std::chrono::nanoseconds ActionSink::getSignalRate() const
{
	return signalRate;
}

uint64_t ActionSink::getOverflowCount() const
{
	updateOverflow();
	return overflowCount;
}

uint64_t ActionSink::getActionCount() const
{
	updateAction();
	return actionCount;
}

uint64_t ActionSink::getLateCount() const
{
	updateLate();
	return lateCount;
}

uint64_t ActionSink::getEarlyCount() const
{
	updateEarly();
	return earlyCount;
}

uint64_t ActionSink::getConflictCount() const
{
	updateConflict();
	return conflictCount;
}

uint64_t ActionSink::getDelayedCount() const
{
	updateDelayed();
	return delayedCount;
}

void ActionSink::setMinOffset(int64_t val)
{
	//ownerOnly();
	minOffset = val;
}

void ActionSink::setMaxOffset(int64_t val)
{
	//ownerOnly();
	maxOffset = val;
}

void ActionSink::setMostFull(uint16_t val)
{
	//ownerOnly();
	if (val != 0) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "can only set MostFull to 0");
	// dev->resetMostFull(channel);
}

void ActionSink::setSignalRate(std::chrono::nanoseconds val)
{
	//ownerOnly();
	signalRate = val;
}

void ActionSink::setOverflowCount(uint64_t val)
{
	//ownerOnly();
	if (val != 0) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "can only set OverflowCount to 0");
	overflowCount = 0;
}

void ActionSink::setActionCount(uint64_t val)
{
	//ownerOnly();
	if (val != 0) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "can only set ActionCount to 0");
	actionCount = 0;
}

void ActionSink::setLateCount(uint64_t val)
{
	//ownerOnly();
	if (val != 0) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "can only set LateCount to 0");
	lateCount = 0;
}

void ActionSink::setEarlyCount(uint64_t val)
{
	//ownerOnly();
	if (val != 0) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "can only set EarlyCount to 0");
	earlyCount = 0;
}

void ActionSink::setConflictCount(uint64_t val)
{
	//ownerOnly();
	if (val != 0) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "can only set ConflictCount to 0");
	conflictCount = 0;
}

void ActionSink::setDelayedCount(uint64_t val)
{
	//ownerOnly();
	if (val != 0) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "can only set DelayedCount to 0");
	delayedCount = 0;
}

void ActionSink::receiveMSI(uint8_t code)
{
	std::cerr << "ActionSink::receiveMSI(" << code << ")" << std::endl;
	std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point exec; 
	std::chrono::milliseconds interval(0);

	switch (code) {
	case ECA_OVERFLOW:
		//DRIVER_LOG("ECA_OVERFLOW",-1, -1);
		saftbus::Loop::get_default().remove(overflowPending); // just to be safe
		exec = overflowUpdate + signalRate;
		if (exec > time) interval = std::chrono::duration_cast<std::chrono::milliseconds>(exec-time);
		overflowPending = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&ActionSink::updateOverflow, this),interval);
		break;
	case ECA_VALID:
		//DRIVER_LOG("ECA_VALID",-1, -1);
		saftbus::Loop::get_default().remove(actionPending); // just to be safe
		exec = actionUpdate + signalRate;
		if (exec > time) interval = std::chrono::duration_cast<std::chrono::milliseconds>(exec-time);
		actionPending = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&ActionSink::updateAction, this),interval);
		break;
	case ECA_LATE:
		//DRIVER_LOG("ECA_LATE",-1, -1);
		saftbus::Loop::get_default().remove(latePending); // just to be safe
		exec = lateUpdate + signalRate;
		if (exec > time) interval = std::chrono::duration_cast<std::chrono::milliseconds>(exec-time);
		latePending = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&ActionSink::updateLate, this),interval);
		break;
	case ECA_EARLY:
		//DRIVER_LOG("ECA_EARLY",-1, -1);
		saftbus::Loop::get_default().remove(earlyPending); // just to be safe
		exec = earlyUpdate + signalRate;
		if (exec > time) interval = std::chrono::duration_cast<std::chrono::milliseconds>(exec-time);
		earlyPending = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&ActionSink::updateEarly, this),interval);
		break;
	case ECA_CONFLICT:
		//DRIVER_LOG("ECA_CONFLICT",-1, -1);
		saftbus::Loop::get_default().remove(conflictPending); // just to be safe
		exec = conflictUpdate + signalRate;
		if (exec > time) interval = std::chrono::duration_cast<std::chrono::milliseconds>(exec-time);
		conflictPending = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&ActionSink::updateConflict, this),interval);
		break;
	case ECA_DELAYED:
		//DRIVER_LOG("ECA_DELAYED",-1, -1);
		saftbus::Loop::get_default().remove(delayedPending); // just to be safe
		exec = delayedUpdate + signalRate;
		if (exec > time) interval = std::chrono::duration_cast<std::chrono::milliseconds>(exec-time);
		delayedPending = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&ActionSink::updateDelayed, this),interval);
		break;
	default:
		//clog << kLogErr << "Asked to handle an invalid MSI condition code in ActionSink.cpp" << std::endl;
		break;
	}
}

bool ActionSink::updateOverflow() const
{
	//DRIVER_LOG("start",-1,-1);
	eb_data_t overflow;

	etherbone::Cycle cycle;
	cycle.open(eca.get_device());
	cycle.write(eca.get_base_address() + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_NUM_SELECT_RW,      EB_DATA32, num);
	// reading OVERFLOW_COUNT clears the count and rearms the MSI
	cycle.read (eca.get_base_address() + ECA_CHANNEL_OVERFLOW_COUNT_GET, EB_DATA32, &overflow);
	cycle.close();

	overflowCount += overflow;

	overflowUpdate = std::chrono::steady_clock::now();
	
	//DRIVER_LOG("done",-1, -1);
	return false;
}

bool ActionSink::updateAction() const
{
	//DRIVER_LOGT("start",name.c_str(),-1,channel);
	eb_data_t valid;

	etherbone::Cycle cycle;
	cycle.open(eca.get_device());
	cycle.write(eca.get_base_address() + ECA_CHANNEL_SELECT_RW,       EB_DATA32, channel);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_NUM_SELECT_RW,   EB_DATA32, num);
	// reading VALID_COUNT clears the count and rearms the MSI
	cycle.read (eca.get_base_address() + ECA_CHANNEL_VALID_COUNT_GET, EB_DATA32, &valid);
	cycle.close();

	actionCount += valid;
	actionUpdate = std::chrono::steady_clock::now();
	//DRIVER_LOG("done",-1,channel);
	return false;
}

ActionSink::Record ActionSink::fetchError(uint8_t code) const
{
	//DRIVER_LOG("start",-1,-1);
	eb_data_t event_hi, event_lo, param_hi, param_lo, tag, tef,
						deadline_hi, deadline_lo, executed_hi, executed_lo, failed;

	etherbone::Cycle cycle;
	cycle.open(eca.get_device());
	cycle.write(eca.get_base_address() + ECA_CHANNEL_SELECT_RW,       EB_DATA32, channel);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_NUM_SELECT_RW,   EB_DATA32, num);
	cycle.write(eca.get_base_address() + ECA_CHANNEL_CODE_SELECT_RW,  EB_DATA32, code);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_EVENT_ID_HI_GET, EB_DATA32, &event_hi);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_EVENT_ID_LO_GET, EB_DATA32, &event_lo);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_PARAM_HI_GET,    EB_DATA32, &param_hi);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_PARAM_LO_GET,    EB_DATA32, &param_lo);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_TAG_GET,         EB_DATA32, &tag);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_TEF_GET,         EB_DATA32, &tef);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_DEADLINE_HI_GET, EB_DATA32, &deadline_hi);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_DEADLINE_LO_GET, EB_DATA32, &deadline_lo);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_EXECUTED_HI_GET, EB_DATA32, &executed_hi);
	cycle.read (eca.get_base_address() + ECA_CHANNEL_EXECUTED_LO_GET, EB_DATA32, &executed_lo);
	// reading FAILED_COUNT clears the count, releases the record, and rearms the MSI
	cycle.read (eca.get_base_address() + ECA_CHANNEL_FAILED_COUNT_GET, EB_DATA32, &failed);
	cycle.close();

	ActionSink::Record out;
	out.event    = uint64_t(event_hi)    << 32 | event_lo;
	out.param    = uint64_t(param_hi)    << 32 | param_lo;
	out.deadline = uint64_t(deadline_hi) << 32 | deadline_lo;
	out.executed = uint64_t(executed_hi) << 32 | executed_lo;
	out.count    = failed;

	//DRIVER_LOG("done",-1,failed);
	return out;
}

bool ActionSink::updateLate() const
{
	//DRIVER_LOG("start",-1, -1);
	Record r = fetchError(ECA_LATE);
	lateCount += r.count;
	lateUpdate = std::chrono::steady_clock::now();
	//DRIVER_LOG("done",-1, -1);
	return false;
}

bool ActionSink::updateEarly() const
{
	//DRIVER_LOG("start",-1, -1);
	Record r = fetchError(ECA_EARLY);
	earlyCount += r.count;
	earlyUpdate = std::chrono::steady_clock::now();
	//DRIVER_LOG("done",-1, -1);
	return false;
}

bool ActionSink::updateConflict() const
{
	//DRIVER_LOG("start",-1, -1);
	Record r = fetchError(ECA_CONFLICT);
	conflictCount += r.count;
	conflictUpdate = std::chrono::steady_clock::now();
	//DRIVER_LOG("done",-1, -1);
	return false;
}

bool ActionSink::updateDelayed() const
{
	//DRIVER_LOG("start",-1, -1);
	Record r = fetchError(ECA_DELAYED);
	delayedCount += r.count;
	delayedUpdate = std::chrono::steady_clock::now();
	//DRIVER_LOG("done",-1, -1);
	return false;
}

void ActionSink::removeCondition(Condition *condition)
{
	std::cerr << "ActionSink::removeCondition(" << (uint64_t)condition << ")" << std::endl;
	// Convert naked pointer into unique_ptr 
	auto found = conditions.find(condition->getNumber());
	if (found != conditions.end()) {
		bool active = found->second->getActive();
		conditions.erase(found);
		try {
			if (active) eca.compile();
		} catch (...) {
			std::cerr << "Failed to recompile after removing condition (should be impossible)" << std::endl;
		}
	} else {
		std::cerr << "ActionSink::removeCondition no condition  " << (uint64_t)condition << " found" << std::endl;
	}
}

void ActionSink::compile()
{
	std::cerr << "ActionSink::compile()" << std::endl;
	eca.compile();
}

const std::string &ActionSink::getObjectName() const 
{ 
	return name; 
}

const std::string &ActionSink::getObjectPath() const 
{ 
	return object_path; 
}

const ActionSink::Conditions& ActionSink::getConditions() const 
{
	return conditions; 
}

unsigned ActionSink::getChannel() const 
{
	return channel; 
}

unsigned ActionSink::getNum() const 
{
	return num; 
}



Condition *ActionSink::getCondition(const std::string object_path) {
	for (auto &condition: conditions) {
		// auto number = pair.first;
		if (condition.second->getObjectPath() == object_path) {
			return condition.second.get();
		}
	}
	return nullptr;
}

unsigned ActionSink::createConditionNumber() {
	for(;;) {
		unsigned number = rand();
		if (conditions.find(number) == conditions.end()) {
			return number;
		}
	}
}



}
