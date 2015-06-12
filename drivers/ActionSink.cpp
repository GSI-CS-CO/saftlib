#define ETHERBONE_THROWS 1

#include "ActionSink.h"

namespace saftlib {

ActionSink::ActionSink(TimingReceiver* dev, int channel)
{
}

void ActionSink::ToggleActive()
{
}

std::vector< Glib::ustring > ActionSink::getAllConditions() const
{
}

std::vector< Glib::ustring > ActionSink::getActiveConditions() const
{
}

std::vector< Glib::ustring > ActionSink::getInactiveConditions() const
{
}

gint64 ActionSink::getMinOffset() const
{
}

gint64 ActionSink::getMaxOffset() const
{
}

guint32 ActionSink::getCapacity() const
{
}

guint32 ActionSink::getFill() const
{
}

guint32 ActionSink::getMostFull() const
{
}

guint32 ActionSink::getOverflowCount() const
{
}

guint32 ActionSink::getConflictCount() const
{
}

bool ActionSink::getExecuteLateActions() const
{
}

guint32 ActionSink::getLateCount() const
{
}

bool ActionSink::getGenerateDelayed() const
{
}

guint32 ActionSink::getDelayedCount() const
{
}

guint32 ActionSink::getActionCount() const
{
}

void ActionSink::setMinOffset(gint64 val)
{
}

void ActionSink::setMaxOffset(gint64 val)
{
}

void ActionSink::setMostFull(guint32 val)
{
}

void ActionSink::setOverflowCount(guint32 val)
{
}

void ActionSink::setConflictCount(guint32 val)
{
}

void ActionSink::setExecuteLateActions(bool val)
{
}

void ActionSink::setLateCount(guint32 val)
{
}

void ActionSink::setGenerateDelayed(bool val)
{
}

void ActionSink::setDelayedCount(guint32 val)
{
}

void ActionSink::setActionCount(guint32 val)
{
}

}
