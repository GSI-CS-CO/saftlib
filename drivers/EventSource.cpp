#define ETHERBONE_THROWS 1

#include "EventSource.h"
#include "clog.h"

namespace saftlib {

EventSource::EventSource(const Glib::ustring& objectPath, TimingReceiver* dev_, const Glib::ustring& name_, sigc::slot<void> destroy)
 : Owned(objectPath, destroy), dev(dev_), name(name_)
{
}

guint64 EventSource::getResolution() const
{
  return 0; // !!!
}

guint32 EventSource::getEventBits() const
{
  return 0; // !!!
}

bool EventSource::getEventEnable() const
{
  return false; // !!!
}

guint64 EventSource::getEventPrefix() const
{
  return 0; // !!!
}
    
void EventSource::setEventEnable(bool val)
{
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

void EventSource::setEventPrefix(guint64 val)
{
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Unimplemented"); // !!!
}

}
