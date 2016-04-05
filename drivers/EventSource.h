#ifndef EVENTSOURCE_H
#define EVENTSOURCE_H

#include "Owned.h"
#include "interfaces/iEventSource.h"

namespace saftlib {

class TimingReceiver;

class EventSource : public Owned, public iEventSource {
  public:
    EventSource(const Glib::ustring& objectPath, TimingReceiver* dev, const Glib::ustring& name, sigc::slot<void> destroy = sigc::slot<void>());
    
    guint64 getResolution() const;
    guint32 getEventBits() const;
    bool getEventEnable() const;
    guint64 getEventPrefix() const;
    
    void setEventEnable(bool val);
    void setEventPrefix(guint64 val);
    
    // Property signals
    //   sigc::signal< void, bool > EventEnable;
    //   sigc::signal< void, guint64 > EventPrefix;
    
    // The name under which this EventSource is listed in TimingReceiver::Iterfaces
    virtual const char *getInterfaceName() const = 0;
    const Glib::ustring &getObjectName() const { return name; }
  
  protected:
    TimingReceiver* dev;
    Glib::ustring name;
};

}

#endif
