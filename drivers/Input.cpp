#define ETHERBONE_THROWS 1

#include "Input.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Input> Input::create(const ConstructorType& args)
{
  return RegisteredObject<Input>::create(args.objectPath, args);
}

Input::Input(const ConstructorType& args) : InoutImpl(args)
{
}

const char *Input::getInterfaceName() const
{
  return "Input";
}

}
