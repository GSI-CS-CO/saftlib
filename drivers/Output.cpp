#define ETHERBONE_THROWS 1

#include "Output.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Output> Output::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<Output>::create(objectPath, args);
}

Output::Output(ConstructorType args) : InoutImpl(args)
{
}

const char *Output::getInterfaceName() const
{
  return "Output";
}

}
