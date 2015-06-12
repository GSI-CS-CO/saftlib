#ifndef SOFTWARE_ACTION_SINK_H
#define SOFTWARE_ACTION_SINK_H

#include "interfaces/SoftwareActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class SoftwareActionSink : public ActionSink, public iSoftwareActionSink, public Glib::Object
{
  public:
    typedef SoftwareActionSink_Service ServiceType;
    struct ConstructorType {
      TimingReceiver* dev;
      int channel;
      sigc::slot<void> destroy;
    };
    
    static Glib::RefPtr<SoftwareActionSink> create(Glib::ustring& objectPath, ConstructorType args);
    
    void destroy();
    
    Glib::ustring NewCondition(bool active, guint64 first, guint64 last, gint64 offset, guint32 guards);
    
  protected:
    SoftwareActionSink(ConstructorType args);
};

}

#endif
