#define ETHERBONE_THROWS 1

#include "Inoutput.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Inoutput> Inoutput::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<Inoutput>::create(objectPath, args);
}

Inoutput::Inoutput(ConstructorType args) : InoutImpl(args)
{
}

const char *Inoutput::getInterfaceName() const
{
  return "Inoutput";
}

}
