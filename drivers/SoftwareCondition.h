#ifndef SOFTWARE_CONDITION_H
#define SOFTWARE_CONDITION_H

#include "interfaces/SoftwareCondition.h"
#include "Condition.h"

namespace saftlib {

class SoftwareCondition : public Condition, public iSoftwareCondition
{
  public:
    typedef SoftwareCondition_Service ServiceType;
    struct ConstructorType {
      ActionSink* sink;
      int channel;
      bool active;
      guint64 first;
      guint64 last;
      guint64 offset;
      guint64 guards;
      guint32 tag;
      sigc::slot<void> destroy;
    };
    
    static Glib::RefPtr<SoftwareCondition> create(Glib::ustring& objectPath, ConstructorType args);
    
    // iSoftwareCondition
    // -> Action
    
  protected:
    SoftwareCondition(ConstructorType args);
};

}

#endif
