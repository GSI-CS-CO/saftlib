#define ETHERBONE_THROWS 1

#include "Input.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Input> Input::create(const ConstructorType& args)
{
  return RegisteredObject<Input>::create(args.objectPath, args);
}

Input::Input(const ConstructorType& args)
 : EventSource(args.objectPath, args.dev, args.name, args.destroy),
   impl(args.impl), partnerPath(args.partnerPath)
{
  impl->StableTime.connect(StableTime.make_slot());
  impl->InputTermination.connect(InputTermination.make_slot());
  impl->SpecialPurposeIn.connect(SpecialPurposeIn.make_slot());
}

const char *Input::getInterfaceName() const
{
  return "Input";
}

bool Input::ReadInput()
{
  return impl->ReadInput();
}

guint32 Input::getStableTime() const 
{
  return impl->getStableTime();
}

bool Input::getInputTermination() const
{
  return impl->getInputTermination();
}

bool Input::getSpecialPurposeIn() const
{
  return impl->getSpecialPurposeIn();
}

bool Input::getInputTerminationAvailable() const
{
  return impl->getInputTerminationAvailable();
}

bool Input::getSpecialPurposeInAvailable() const
{
  return impl->getSpecialPurposeInAvailable();
}

Glib::ustring Input::getLogicLevelIn() const
{
  return impl->getLogicLevelIn();
}

Glib::ustring Input::getOutput() const
{
  return partnerPath;
}

void Input::setStableTime(guint32 val)
{
  ownerOnly();
  return impl->setStableTime(val);
}

void Input::setInputTermination(bool val)
{
  ownerOnly();
  return impl->setInputTermination(val);
}

void Input::setSpecialPurposeIn(bool val)
{
  ownerOnly();
  return impl->setSpecialPurposeIn(val);
}

}
