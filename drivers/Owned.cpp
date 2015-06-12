#define ETHERBONE_THROWS 1

#include "Owned.h"

namespace saftlib {

Owned::Owned(sigc::slot<void> destroy_)
 : destroy(destroy_)
{
}

void Owned::Disown()
{
  if (owner.empty()) {
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Do not have an Owner");
  } else {
    owner.clear();
    Owner(owner);
  }
}

void Owned::Own()
{
  if (owner.empty()) {
    owner = getSender();
    Owner(owner);
  } else {
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Already have an Owner");
  }
}

void Owned::Destroy()
{
  if (!getDestructible())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Attempt to Destroy non-Destructible Owned object");
  
  owner_only();
  destroy();
}

Glib::ustring Owned::getOwner() const
{
  return owner;
}

bool Owned::getDestructible() const
{
  return !destroy.empty();
}

void Owned::owner_only() const
{
  if (!owner.empty() && owner != getSender())
    throw Gio::DBus::Error(Gio::DBus::Error::ACCESS_DENIED, "You are not my Owner");
}

}
