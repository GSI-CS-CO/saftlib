#ifndef OUTPUT_H
#define OUTPUT_H

#include "InoutImpl.h"
#include "interfaces/Output.h"

namespace saftlib {

class Output : public InoutImpl
{
  public:
    typedef Output_Service ServiceType;

    static Glib::RefPtr<Output> create(const Glib::ustring& objectPath, ConstructorType args);
    
    const char *getInterfaceName() const;
    
  protected:
    Output(ConstructorType args);
};

}

#endif /* OUTPUT_H */
