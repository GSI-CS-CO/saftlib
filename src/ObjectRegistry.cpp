#define ETHERBONE_THROWS 1

#include <list>
#include "ObjectRegistry.h"

namespace saftlib {

static std::list<RegisteredObjectBase*> live_set;
static Glib::RefPtr<Gio::DBus::Connection> current_connection;

RegisteredObjectBase::~RegisteredObjectBase()
{
}

void RegisteredObjectBase::insert_self()
{
  index = live_set.insert(live_set.begin(), this);
  if (current_connection)
    register_self(current_connection);
}

void RegisteredObjectBase::remove_self()
{
  unregister_self();
  live_set.erase(index);
}

void ObjectRegistry::register_all(const Glib::RefPtr<Gio::DBus::Connection>& connection)
{
  current_connection = connection;
  for (std::list<RegisteredObjectBase*>::const_iterator i = live_set.begin(); i != live_set.end(); ++i)
    (*i)->register_self(connection);
}

void ObjectRegistry::unregister_all()
{
  current_connection.reset();
  for (std::list<RegisteredObjectBase*>::const_iterator i = live_set.begin(); i != live_set.end(); ++i)
    (*i)->unregister_self();
}

} // saftlib
