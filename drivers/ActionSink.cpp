#define ETHERBONE_THROWS 1

#include <iostream>

#include "ActionSink.h"
#include "TimingReceiver.h"

namespace saftlib {

ActionSink::ActionSink(TimingReceiver* dev_, int channel_, sigc::slot<void> destroy)
 : Owned(destroy), dev(dev_), channel(channel_), cond_count(0), 
   minOffset(-100000),  maxOffset(1000000000L), executeLateActions(false), generateDelayed(false)
{
}

ActionSink::~ActionSink()
{
  try {
    conditions.clear();
    dev->compile();
  } catch (...) {
    // !!! do something to prevent this
    std::cerr << "Failed to recompile during ActionSink destruction" << std::endl;
  }
}

void ActionSink::ToggleActive()
{
  ownerOnly();

  // Toggle raw to prevent recompile at each step
  std::list< Glib::RefPtr<Condition> >::iterator i;
  for (i = conditions.begin(); i != conditions.end(); ++i)
    (*i)->setRawActive(!(*i)->getActive());
  
  try {
    compile();
  } catch (...) {
    // failed => undo flips
    for (i = conditions.begin(); i != conditions.end(); ++i)
      (*i)->setRawActive(!(*i)->getActive());
    throw;
  }
  
  // notify changes
  for (i = conditions.begin(); i != conditions.end(); ++i)
    (*i)->Active((*i)->getActive());
  notify(true, true);
}

std::vector< Glib::ustring > ActionSink::getAllConditions() const
{
  std::vector< Glib::ustring > out;
  std::list< Glib::RefPtr<Condition> >::const_iterator i;
  for (i = conditions.begin(); i != conditions.end(); ++i)
    out.push_back((*i)->getObjectPath());
  return out;
}

std::vector< Glib::ustring > ActionSink::getActiveConditions() const
{
  std::vector< Glib::ustring > out;
  std::list< Glib::RefPtr<Condition> >::const_iterator i;
  for (i = conditions.begin(); i != conditions.end(); ++i)
    if ((*i)->getActive()) out.push_back((*i)->getObjectPath());
  return out;
}

std::vector< Glib::ustring > ActionSink::getInactiveConditions() const
{
  std::vector< Glib::ustring > out;
  std::list< Glib::RefPtr<Condition> >::const_iterator i;
  for (i = conditions.begin(); i != conditions.end(); ++i)
    if (!(*i)->getActive()) out.push_back((*i)->getObjectPath());
  return out;
}

gint64 ActionSink::getMinOffset() const
{
  return minOffset;
}

gint64 ActionSink::getMaxOffset() const
{
  return maxOffset;
}

guint32 ActionSink::getCapacity() const
{
  return dev->getQueueSize();
}

guint32 ActionSink::getFill() const
{
  return 0; // !!!
}

guint32 ActionSink::getMostFull() const
{
  return 0; // !!!
}

guint32 ActionSink::getOverflowCount() const
{
  return 0; // !!!
}

guint32 ActionSink::getConflictCount() const
{
  return 0; // !!!
}

bool ActionSink::getExecuteLateActions() const
{
  return executeLateActions;
}

guint32 ActionSink::getLateCount() const
{
  return 0; // !!!
}

bool ActionSink::getGenerateDelayed() const
{
  return generateDelayed;
}

guint32 ActionSink::getDelayedCount() const
{
  return 0; // !!!
}

guint32 ActionSink::getActionCount() const
{
  return 0; // !!!
}

void ActionSink::setMinOffset(gint64 val)
{
  ownerOnly();
  minOffset = val;
  MinOffset(minOffset);
}

void ActionSink::setMaxOffset(gint64 val)
{
  ownerOnly();
  maxOffset = val;
  MaxOffset(maxOffset);
}

void ActionSink::setMostFull(guint32 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::setOverflowCount(guint32 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::setConflictCount(guint32 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::setExecuteLateActions(bool val)
{
  ownerOnly();
//  executeLateActions = val;
//  ExecuteLateActions(executeLateActions);
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::setLateCount(guint32 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::setGenerateDelayed(bool val)
{
  ownerOnly();
//  generateDelayed = val;
//  GenerateDelayed(generateDelayed);
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::setDelayedCount(guint32 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::setActionCount(guint32 val)
{
  ownerOnly();
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void ActionSink::removeCondition(std::list< Glib::RefPtr<Condition> >::iterator i)
{
  bool active = (*i)->getActive();
  conditions.erase(i);
  try {
    notify(active, !active);
    dev->compile();
  } catch (...) {
    // !!! do something to prevent this
    std::cerr << "Failed to recompile after removing condition" << std::endl;
  }
}

void ActionSink::notify(bool active, bool inactive)
{
  AllConditions(getAllConditions());
  if (active)   ActiveConditions(getActiveConditions());
  if (inactive) InactiveConditions(getInactiveConditions());
}

void ActionSink::compile()
{
  dev->compile();
}

Glib::ustring ActionSink::NewConditionHelper(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards, guint32 tag, ConditionConstructor constructor)
{
  ownerOnly();
  
  // sanity check arguments
  if (offset < minOffset || offset > maxOffset)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "offset is out of range; adjust {min,max}Offset?");
  if (~mask & (~mask+1) != 0)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "mask is not a prefix");
  if (id & mask != id)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "id has bits set that are not in the mask");
  if (guards)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "guard requested which is unavailable in hardware");
  
  // Make a space for it in the container 
  conditions.push_back(Glib::RefPtr<Condition>());
  sigc::slot<void> destroy = sigc::bind(sigc::mem_fun(this, &ActionSink::removeCondition), --conditions.end());
  
  std::ostringstream str;
  str.imbue(std::locale("C"));
  str << getObjectPath() << "/_" << ++cond_count;
  Glib::ustring path = str.str();
  
  Glib::RefPtr<Condition> condition;
  try {
    Condition::Condition_ConstructorType args = { this, channel, active, id, mask, offset, guards, tag, destroy };
    condition = constructor(path, args);
    condition->initOwner(getConnection(), getSender());
    conditions.back() = condition;
    if (active) compile();
  } catch (...) {
    destroy();
    throw;
  }
  
  notify(active, !active);
  return condition->getObjectPath();
}

}
