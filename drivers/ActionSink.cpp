#define ETHERBONE_THROWS 1

#include <iostream>

#include "ActionSink.h"
#include "TimingReceiver.h"

namespace saftlib {

ActionSink::ActionSink(TimingReceiver* dev_, int channel_, sigc::slot<void> destroy)
 : Owned(destroy), dev(dev), channel(channel_), cond_count(0), 
   minOffset(-100000),  maxOffset(1000000000L), executeLateActions(false), generateDelayed(false)
{
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
  out;
}

std::vector< Glib::ustring > ActionSink::getActiveConditions() const
{
  std::vector< Glib::ustring > out;
  std::list< Glib::RefPtr<Condition> >::const_iterator i;
  for (i = conditions.begin(); i != conditions.end(); ++i)
    if ((*i)->getActive()) out.push_back((*i)->getObjectPath());
  out;
}

std::vector< Glib::ustring > ActionSink::getInactiveConditions() const
{
  std::vector< Glib::ustring > out;
  std::list< Glib::RefPtr<Condition> >::const_iterator i;
  for (i = conditions.begin(); i != conditions.end(); ++i)
    if (!(*i)->getActive()) out.push_back((*i)->getObjectPath());
  out;
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
  return 256; // !!!
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

}
