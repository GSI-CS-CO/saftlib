#ifndef INPUT_H
#define INPUT_H

#include "InoutImpl.h"
#include "interfaces/Input.h"

namespace saftlib {

class Input : public InoutImpl
{
  public:
    typedef Input_Service ServiceType;

    static Glib::RefPtr<Input> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
  protected:
    Input(const ConstructorType& args);
};

}

#endif /* INPUT_H */
