#ifndef INOUTPUT_H
#define INOUTPUT_H

#include "InoutImpl.h"
#include "interfaces/Inoutput.h"

namespace saftlib {

class Inoutput : public InoutImpl
{
  public:
    typedef Inoutput_Service ServiceType;

    static Glib::RefPtr<Inoutput> create(const Glib::ustring& objectPath, ConstructorType args);
    
    const char *getInterfaceName() const;
    
  protected:
    Inoutput(ConstructorType args);
};

}

#endif /* INOUTPUT_H */
