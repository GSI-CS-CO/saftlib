#ifndef SOFTWARE_ACTION_SINK_H
#define SOFTWARE_ACTION_SINK_H

#include "interfaces/SoftwareActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class SoftwareActionSink : public ActionSink, public iSoftwareActionSink
{
  public:
    typedef SoftwareActionSink_Service ServiceType;
    struct ConstructorType {
      TimingReceiver* dev;
      Glib::ustring name;
      unsigned channel;
      unsigned num;
      eb_address_t queue;
      sigc::slot<void> destroy;
    };
    
    static Glib::RefPtr<SoftwareActionSink> create(const Glib::ustring& objectPath, ConstructorType args);
    
    const char *getInterfaceName() const;
    
    // override receiveMSI to also pop the software queue
    void receiveMSI(guint8 code);
    
    // iSoftwareAcitonSink
    Glib::ustring NewCondition(bool active, guint64 id, guint64 mask, gint64 offset);
    
  protected:
    SoftwareActionSink(ConstructorType args);
    eb_address_t queue;
};

}

#endif
