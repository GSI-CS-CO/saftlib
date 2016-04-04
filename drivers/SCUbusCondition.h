#ifndef SCUBUS_CONDITION_H
#define SCUBUS_CONDITION_H

#include "interfaces/SCUbusCondition.h"
#include "Condition.h"

namespace saftlib {

class SCUbusCondition : public Condition, public iSCUbusCondition
{
  public:
    typedef SCUbusCondition_Service ServiceType;
    typedef Condition_ConstructorType ConstructorType;
    
    static Glib::RefPtr<SCUbusCondition> create(const Glib::ustring& objectPath, ConstructorType args);
    
    // iSCUbusCondition
    guint32 getTag() const;
    void setTag(guint32 val);
    
  protected:
    SCUbusCondition(ConstructorType args);
};

}

#endif
