#define ETHERBONE_THROWS 1

#include "Inoutput.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Inoutput> Inoutput::create(const ConstructorType& args)
{
  return RegisteredObject<Inoutput>::create(args.objectPath, args);
}

Inoutput::Inoutput(const ConstructorType& args) : InoutImpl(args)
{
}

const char *Inoutput::getInterfaceName() const
{
  return "Inoutput";
}

}
