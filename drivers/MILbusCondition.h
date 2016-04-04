#ifndef MILBUS_CONDITION_H
#define MILBUS_CONDITION_H

#include "interfaces/MILbusCondition.h"
#include "Condition.h"

namespace saftlib {

class MILbusCondition : public Condition, public iMILbusCondition
{
  public:
    typedef MILbusCondition_Service ServiceType;
    typedef Condition_ConstructorType ConstructorType;
    
    static Glib::RefPtr<MILbusCondition> create(const ConstructorType& args);
    
    // iMILbusCondition
    guint16 getTag() const;
    void setTag(guint16 val);
    
  protected:
    MILbusCondition(const ConstructorType& args);
};

}

#endif
