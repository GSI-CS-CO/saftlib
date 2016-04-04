#ifndef MILBUS_ACTION_SINK_H
#define MILBUS_ACTION_SINK_H

#include "interfaces/MILbusActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class MILbusActionSink : public ActionSink, public iMILbusActionSink
{
  public:
    typedef MILbusActionSink_Service ServiceType;
    struct ConstructorType {
      Glib::ustring objectPath;
      TimingReceiver* dev;
      Glib::ustring name;
      unsigned channel;
    };
    
    static Glib::RefPtr<MILbusActionSink> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // iMILbusAcitonSink
    Glib::ustring NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint16 tag);
    void InjectTag(guint16 tag);
    
  protected:
    MILbusActionSink(const ConstructorType& args);
};

}

#endif
