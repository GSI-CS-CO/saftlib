#include "GlobalObject.h"

namespace saftlib {

typedef std::list<GlobalObject*> live_set;
static live_set live;

static Glib::RefPtr<Gio::DBus::Connection> connection;

GlobalObject::GlobalObject() 
 : index(live.insert(live.begin(), this))
{
//  if (connection)
//    register_self(connection);
}

GlobalObject::~GlobalObject()
{
  live.erase(index);
}

void GlobalObject::register_all(const Glib::RefPtr<Gio::DBus::Connection>& connection)
{
  for (live_set::const_iterator i = live.begin(); i != live.end(); ++i)
    (*i)->register_self(connection);
}

void GlobalObject::unregister_all()
{
  for (live_set::const_iterator i = live.begin(); i != live.end(); ++i)
    (*i)->unregister_self();
}

}
