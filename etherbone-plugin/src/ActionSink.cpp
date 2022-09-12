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
#include "TimingReceiver.hpp"
#include "eca_regs.h"
#include "eca_flags.h"
//#include "clog.h"

#include <saftbus/error.hpp>

namespace eb_plugin {

ActionSink::ActionSink(const std::string& objectPath, TimingReceiver* dev_, const std::string& name_, unsigned channel_, unsigned num_)//, sigc::slot<void> destroy)
 : object_path(objectPath), 
 dev(dev_), name(name_), channel(channel_), num(num_),
   minOffset(-1000000000L),  maxOffset(1000000000L), signalRate(std::chrono::nanoseconds(100000000L)),
   overflowCount(0), actionCount(0), lateCount(0), earlyCount(0), conflictCount(0), delayedCount(0)
{
  overflowUpdate = actionUpdate = lateUpdate = earlyUpdate = conflictUpdate = delayedUpdate = std::chrono::steady_clock::now();
  eb_data_t raw_latency, raw_offset_bits, raw_capacity, null;

  etherbone::Cycle cycle;
  cycle.open(dev->getDevice());
  // Grab configuration
  cycle.write(dev->getBase() + ECA_CHANNEL_SELECT_RW,     EB_DATA32, channel);
  cycle.write(dev->getBase() + ECA_CHANNEL_NUM_SELECT_RW, EB_DATA32, num);
  cycle.read (dev->getBase() + ECA_LATENCY_GET,           EB_DATA32, &raw_latency);
  cycle.read (dev->getBase() + ECA_OFFSET_BITS_GET,       EB_DATA32, &raw_offset_bits);
  cycle.read (dev->getBase() + ECA_CHANNEL_CAPACITY_GET,  EB_DATA32, &raw_capacity);
  // Wipe out any old state:
  cycle.read (dev->getBase() + ECA_CHANNEL_OVERFLOW_COUNT_GET, EB_DATA32, &null);
  cycle.read (dev->getBase() + ECA_CHANNEL_VALID_COUNT_GET,    EB_DATA32, &null);
  cycle.write(dev->getBase() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_LATE);
  cycle.read (dev->getBase() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
  cycle.write(dev->getBase() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_EARLY);
  cycle.read (dev->getBase() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
  cycle.write(dev->getBase() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_CONFLICT);
  cycle.read (dev->getBase() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
  cycle.write(dev->getBase() + ECA_CHANNEL_CODE_SELECT_RW,     EB_DATA32, ECA_DELAYED);
  cycle.read (dev->getBase() + ECA_CHANNEL_FAILED_COUNT_GET,   EB_DATA32, &null);
  cycle.close();

  latency = raw_latency;   // in nanoseconds
  earlyThreshold = 1;      // in nanoseconds
  earlyThreshold <<= raw_offset_bits;
  capacity = raw_capacity; // in actions
}

ActionSink::~ActionSink()
{
  // unhook any pending updates
  saftbus::Loop::get_default().remove(overflowPending);
  saftbus::Loop::get_default().remove(actionPending);
  saftbus::Loop::get_default().remove(latePending);
  saftbus::Loop::get_default().remove(earlyPending);
  saftbus::Loop::get_default().remove(conflictPending);
  saftbus::Loop::get_default().remove(delayedPending);

  // No need to recompile; done in TimingReceiver.cpp
}

void ActionSink::ToggleActive()
{
  // //ownerOnly();

  // // Toggle raw to prevent recompile at each step
  // Conditions::iterator i;
  // for (i = conditions.begin(); i != conditions.end(); ++i)
  //   i->second->setRawActive(!i->second->getActive());

  // try {
  //   compile();
  // } catch (...) {
  //   // failed => undo flips
  //   for (i = conditions.begin(); i != conditions.end(); ++i)
  //     i->second->setRawActive(!i->second->getActive());
  //   throw;
  // }
}

uint16_t ActionSink::ReadFill()
{
  // return dev->updateMostFull(channel);
  return 0;
}

std::vector< std::string > ActionSink::getAllConditions() const
{
  std::vector< std::string > out;
  // Conditions::const_iterator i;
  // for (i = conditions.begin(); i != conditions.end(); ++i)
  //   out.push_back(i->second->getObjectPath());
  return out;
}

std::vector< std::string > ActionSink::getActiveConditions() const
{
  std::vector< std::string > out;
  // Conditions::const_iterator i;
  // for (i = conditions.begin(); i != conditions.end(); ++i)
  //   if (i->second->getActive()) out.push_back(i->second->getObjectPath());
  return out;
}

std::vector< std::string > ActionSink::getInactiveConditions() const
{
  std::vector< std::string > out;
  // Conditions::const_iterator i;
  // for (i = conditions.begin(); i != conditions.end(); ++i)
  //   if (!i->second->getActive()) out.push_back(i->second->getObjectPath());
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
  return dev->most_full[channel];
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
  cycle.open(dev->getDevice());
  cycle.write(dev->getBase() + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
  cycle.write(dev->getBase() + ECA_CHANNEL_NUM_SELECT_RW,      EB_DATA32, num);
  // reading OVERFLOW_COUNT clears the count and rearms the MSI
  cycle.read (dev->getBase() + ECA_CHANNEL_OVERFLOW_COUNT_GET, EB_DATA32, &overflow);
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
  cycle.open(dev->getDevice());
  cycle.write(dev->getBase() + ECA_CHANNEL_SELECT_RW,       EB_DATA32, channel);
  cycle.write(dev->getBase() + ECA_CHANNEL_NUM_SELECT_RW,   EB_DATA32, num);
  // reading VALID_COUNT clears the count and rearms the MSI
  cycle.read (dev->getBase() + ECA_CHANNEL_VALID_COUNT_GET, EB_DATA32, &valid);
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
  cycle.open(dev->getDevice());
  cycle.write(dev->getBase() + ECA_CHANNEL_SELECT_RW,       EB_DATA32, channel);
  cycle.write(dev->getBase() + ECA_CHANNEL_NUM_SELECT_RW,   EB_DATA32, num);
  cycle.write(dev->getBase() + ECA_CHANNEL_CODE_SELECT_RW,  EB_DATA32, code);
  cycle.read (dev->getBase() + ECA_CHANNEL_EVENT_ID_HI_GET, EB_DATA32, &event_hi);
  cycle.read (dev->getBase() + ECA_CHANNEL_EVENT_ID_LO_GET, EB_DATA32, &event_lo);
  cycle.read (dev->getBase() + ECA_CHANNEL_PARAM_HI_GET,    EB_DATA32, &param_hi);
  cycle.read (dev->getBase() + ECA_CHANNEL_PARAM_LO_GET,    EB_DATA32, &param_lo);
  cycle.read (dev->getBase() + ECA_CHANNEL_TAG_GET,         EB_DATA32, &tag);
  cycle.read (dev->getBase() + ECA_CHANNEL_TEF_GET,         EB_DATA32, &tef);
  cycle.read (dev->getBase() + ECA_CHANNEL_DEADLINE_HI_GET, EB_DATA32, &deadline_hi);
  cycle.read (dev->getBase() + ECA_CHANNEL_DEADLINE_LO_GET, EB_DATA32, &deadline_lo);
  cycle.read (dev->getBase() + ECA_CHANNEL_EXECUTED_HI_GET, EB_DATA32, &executed_hi);
  cycle.read (dev->getBase() + ECA_CHANNEL_EXECUTED_LO_GET, EB_DATA32, &executed_lo);
  // reading FAILED_COUNT clears the count, releases the record, and rearms the MSI
  cycle.read (dev->getBase() + ECA_CHANNEL_FAILED_COUNT_GET, EB_DATA32, &failed);
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

// void ActionSink::removeCondition(Conditions::iterator i)
// {
//   bool active = i->second->getActive();
//   conditions.erase(i);
//   try {
//     if (active) dev->compile();
//   } catch (...) {
//     //clog << kLogErr << "Failed to recompile after removing condition (should be impossible)" << std::endl;
//   }
// }

void ActionSink::compile()
{
  dev->compile();
}

// std::string ActionSink::NewConditionHelper(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, bool tagIsKey, ConditionConstructor constructor)
// {
//   //ownerOnly();

//   // sanity check arguments
//   if (offset < minOffset || offset > maxOffset)
//     throw saftbus::Error(saftbus::Error::INVALID_ARGS, "offset is out of range; adjust {min,max}Offset?");
//   if ((~mask & (~mask+1)) != 0)
//     throw saftbus::Error(saftbus::Error::INVALID_ARGS, "mask is not a prefix");
//   if ((id & mask) != id)
//     throw saftbus::Error(saftbus::Error::INVALID_ARGS, "id has bits set that are not in the mask");

//   // Pick a random number
//   std::pair<Conditions::iterator, bool> attempt;
//   do attempt = conditions.insert(Conditions::value_type(random(), std::shared_ptr<Condition>()));
//   while (!attempt.second);

//   // Setup a destruction callback
//   sigc::slot<void> destroy = sigc::bind(sigc::mem_fun(this, &ActionSink::removeCondition), attempt.first);

//   std::ostringstream str;
//   str.imbue(std::locale("C"));
//   str << getObjectPath() << "/_" << attempt.first->first;
//   std::string path = str.str();

//   std::shared_ptr<Condition> condition;
//   try {
//     Condition::Condition_ConstructorType args = {
//       path, this, active, id, mask, offset, tagIsKey?attempt.first->first:tag, destroy
//     };
//     condition = constructor(args);
//     condition->initOwner(getConnection(), getSender());
//     attempt.first->second = condition;
//     if (active) compile();
//   } catch (...) {
//     destroy();
//     throw;
//   }

//   return condition->getObjectPath();
// }

}
