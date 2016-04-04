#define ETHERBONE_THROWS 1

#include "Output.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Output> Output::create(const ConstructorType& args)
{
  return RegisteredObject<Output>::create(args.objectPath, args);
}

Output::Output(const ConstructorType& args) : InoutImpl(args)
{
}

const char *Output::getInterfaceName() const
{
  return "Output";
}

}
