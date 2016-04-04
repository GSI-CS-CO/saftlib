#define ETHERBONE_THROWS 1

#include "Owned.h"
#include "clog.h"
#include "Device.h"

namespace saftlib {

static void do_unsubscribe(Glib::RefPtr<Gio::DBus::Connection> connection, guint id) 
{
  connection->signal_unsubscribe(id);
}

Owned::Owned(const Glib::ustring& objectPath, sigc::slot<void> destroy_)
 : BaseObject(objectPath), destroy(destroy_)
{
}

Owned::~Owned()
{
  try {
    Destroyed(); 
    if (!owner.empty()) unsubscribe();
  } catch (...) { 
    clog << kLogErr << "Owned::~Owned:: threw an exception!" << std::endl;
  }
}

void Owned::Disown()
{
  if (owner.empty()) {
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Do not have an Owner");
  } else {
    ownerOnly();
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
        sigc::bind(sigc::ptr_fun(&Owned::owner_quit_handler), this),
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
  const Glib::ustring&, const Glib::VariantContainerBase&,
  Owned* self)
{
  try {
    self->unsubscribe();
    self->owner.clear();
    self->Owner(self->owner);
    self->ownerQuit(); // inform base classes, in case they have clean-up code
    
    if (self->getDestructible()) self->destroy();
    // do not use self beyond this point
  } catch (const etherbone::exception_t& e) {
    clog << kLogErr << "Owned::owner_quit_handler: " << e << std::endl; 
  } catch (const Glib::Error& e) {           
    clog << kLogErr << "Owned::owner_quit_handler: " << e.what() << std::endl; 
  } catch (...) {
    clog << kLogErr << "Owned::owner_quit_handler: unknown exception" << std::endl;
  }
}

}
