#ifndef OUTPUT_CONDITION_H
#define OUTPUT_CONDITION_H

#include "interfaces/OutputCondition.h"
#include "Condition.h"

namespace saftlib {

class OutputCondition : public Condition, public iOutputCondition
{
  public:
    typedef OutputCondition_Service ServiceType;
    typedef Condition_ConstructorType ConstructorType;
    
    static Glib::RefPtr<OutputCondition> create(const Glib::ustring& objectPath, ConstructorType args);
    
    // iOutputCondition
    bool getOn() const;
    void setOn(bool val);
    
  protected:
    OutputCondition(ConstructorType args);
};

}

#endif
