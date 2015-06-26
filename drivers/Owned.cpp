#define ETHERBONE_THROWS 1

#include "Owned.h"

namespace saftlib {

static void do_unsubscribe(Glib::RefPtr<Gio::DBus::Connection> connection, guint id) 
{
  connection->signal_unsubscribe(id);
}

Owned::Owned(sigc::slot<void> destroy_)
 : destroy(destroy_)
{
}

Owned::~Owned()
{
  if (!owner.empty()) unsubscribe();
}

void Owned::Disown()
{
  if (owner.empty()) {
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Do not have an Owner");
  } else {
    unsubscribe();
    owner.clear();
    Owner(owner);
  }
}

void Owned::Own()
{
  initOwner(getConnection(), getSender());
}

void Owned::initOwner(const Glib::RefPtr<Gio::DBus::Connection>& connection_, const Glib::ustring& owner_)
{
  if (owner.empty()) {
    owner = owner_;
    Glib::RefPtr<Gio::DBus::Connection> connection = connection_;
    guint subscription_id = connection->signal_subscribe(
        sigc::mem_fun(this, &Owned::owner_quit_handler),
        "org.freedesktop.DBus",
        "org.freedesktop.DBus",
        "NameOwnerChanged",
        "/org/freedesktop/DBus",
        owner);
    unsubscribe = sigc::bind(sigc::ptr_fun(&do_unsubscribe), connection, subscription_id);
    Owner(owner);
  } else {
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Already have an Owner");
  }
}

void Owned::Destroy()
{
  if (!getDestructible())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Attempt to Destroy non-Destructible Owned object");
  
  ownerOnly();
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

void Owned::ownerOnly() const
{
  if (!owner.empty() && owner != getSender())
    throw Gio::DBus::Error(Gio::DBus::Error::ACCESS_DENIED, "You are not my Owner");
}

void Owned::ownerQuit()
{
}

void Owned::owner_quit_handler(
  const Glib::RefPtr<Gio::DBus::Connection>&,
  const Glib::ustring&, const Glib::ustring&, const Glib::ustring&,
  const Glib::ustring&, const Glib::VariantContainerBase&)
{
  unsubscribe();
  owner.clear();
  Owner(owner);
  ownerQuit(); // inform base classes, in case they have clean-up code
  if (getDestructible()) destroy();
}

}
