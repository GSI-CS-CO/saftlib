#define ETHERBONE_THROWS 1

#include "Output.h"
#include "OutputCondition.h"
#include "RegisteredObject.h"

namespace saftlib {

Glib::RefPtr<Output> Output::create(const ConstructorType& args)
{
  return RegisteredObject<Output>::create(args.objectPath, args);
}

Output::Output(const ConstructorType& args) : 
  ActionSink(args.objectPath, args.dev, args.name, args.channel, args.num, args.destroy),
  impl(args.impl), partnerPath(args.partnerPath)
{
  impl->OutputEnable.connect(OutputEnable.make_slot());
  impl->SpecialPurposeOut.connect(SpecialPurposeOut.make_slot());
  impl->BuTiSMultiplexer.connect(BuTiSMultiplexer.make_slot());
}

const char *Output::getInterfaceName() const
{
  return "Output";
}

Glib::ustring Output::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, bool on)
{
  return NewConditionHelper(active, id, mask, offset, on?1:2, false,
    sigc::ptr_fun(&OutputCondition::create));
}

void Output::WriteOutput(bool value)
{
  ownerOnly();
  return impl->WriteOutput(value);
}

bool Output::ReadOutput()
{
  return impl->ReadOutput();
}

bool Output::getOutputEnable() const
{
  return impl->getOutputEnable();
}

bool Output::getSpecialPurposeOut() const
{
  return impl->getSpecialPurposeOut();
}

bool Output::getBuTiSMultiplexer() const
{
  return impl->getBuTiSMultiplexer();
}

bool Output::getOutputEnableAvailable() const
{
  return impl->getOutputEnableAvailable();
}

bool Output::getSpecialPurposeOutAvailable() const
{
  return impl->getSpecialPurposeOutAvailable();
}

Glib::ustring Output::getLogicLevelOut() const
{
  return impl->getLogicLevelOut();
}

Glib::ustring Output::getInput() const
{
  return partnerPath;
}

void Output::setOutputEnable(bool val)
{
  ownerOnly();
  return impl->setOutputEnable(val);
}

void Output::setSpecialPurposeOut(bool val)
{
  ownerOnly();
  return impl->setSpecialPurposeOut(val);
}

void Output::setBuTiSMultiplexer(bool val)
{
  ownerOnly();
  return impl->setBuTiSMultiplexer(val);
}

}
