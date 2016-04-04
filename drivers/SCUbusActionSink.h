#ifndef SCUBUS_ACTION_SINK_H
#define SCUBUS_ACTION_SINK_H

#include "interfaces/SCUbusActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class SCUbusActionSink : public ActionSink, public iSCUbusActionSink
{
  public:
    typedef SCUbusActionSink_Service ServiceType;
    struct ConstructorType {
      Glib::ustring objectPath;
      TimingReceiver* dev;
      Glib::ustring name;
      unsigned channel;
      eb_address_t scubus;
    };
    
    static Glib::RefPtr<SCUbusActionSink> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // iSCUbusAcitonSink
    Glib::ustring NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 tag);
    void InjectTag(guint32 tag);
    
  protected:
    SCUbusActionSink(const ConstructorType& args);
    eb_address_t scubus;
};

}

#endif
