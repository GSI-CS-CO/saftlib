#define ETHERBONE_THROWS 1

#include "Input.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Input> Input::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<Input>::create(objectPath, args);
}

Input::Input(ConstructorType args) : InoutImpl(args)
{
}

const char *Input::getInterfaceName() const
{
  return "Input";
}

}
