#ifndef OBJECT_H
#define OBJECT_H

#include <giomm.h>

namespace saftlib {

class BaseObject : public Glib::Object
{
  public:
    BaseObject(const Glib::ustring& objectPath);
    
    // Most classes need this to build paths recursively
    const Glib::ustring& getObjectPath() const { return objectPath; }
    
    // provided by RegisteredObject
    virtual const Glib::ustring& getSender() const = 0;
    virtual const Glib::RefPtr<Gio::DBus::Connection>& getConnection() const = 0;
    
  protected:
    Glib::ustring objectPath;
};

}

#endif
