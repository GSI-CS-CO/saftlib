#ifndef ECA_TOP_OBJECT_H
#define ECA_TOP_OBJECT_H

#include <giomm.h>
#include <list>

namespace saftlib {

class GlobalObject {
  public:
    GlobalObject();
    virtual ~GlobalObject();
    
    // These functions may not create/destroy GlobalObjects during their invocation!
    virtual void register_self(const Glib::RefPtr<Gio::DBus::Connection>& connection) = 0;
    virtual void unregister_self() = 0;
    
    static void register_all(const Glib::RefPtr<Gio::DBus::Connection>& connection);
    static void unregister_all();
  
  private:
    typedef std::list<GlobalObject*> live_set;
    live_set::iterator index;
};

}

#endif
