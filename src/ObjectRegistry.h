#ifndef SAFTLIB_OBJECT_REGISTRY_H
#define SAFTLIB_OBJECT_REGISTRY_H

#include <list>
#include <giomm.h>
#include <etherbone.h>

namespace saftlib {

class RegisteredObjectBase
{
  public:
    RegisteredObjectBase() { }
    virtual ~RegisteredObjectBase();
    
  protected:
    // These functions may not create/destroy RegisteredObjects during their invocation!
    virtual void register_self(const Glib::RefPtr<Gio::DBus::Connection>& connection) = 0;
    virtual void unregister_self() = 0;
    
    void insert_self();
    void remove_self();

  private:
    // secret
    std::list<RegisteredObjectBase*>::iterator index;
    
    // non-copyable
    RegisteredObjectBase(const RegisteredObjectBase&);
    RegisteredObjectBase& operator = (const RegisteredObjectBase&);
  
  friend class ObjectRegistry;
};

template <typename T>
class RegisteredObject : public T, private RegisteredObjectBase
{
  public:
    RegisteredObject(const Glib::ustring& object_path) : T(object_path) { insert_self(); }
    ~RegisteredObject() { remove_self(); }
    
    void rethrow(const char *method);
  
  private:
    void register_self(const Glib::RefPtr<Gio::DBus::Connection>& connection) { T::register_self(connection); }
    void unregister_self() { T::unregister_self(); }
};

template <typename T>
void RegisteredObject<T>::rethrow(const char *method)
{
  try {
    T::rethrow(method);
  } catch (const etherbone::exception_t& e) {
    std::ostringstream str;
    str << method << ": " << e;
    throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, str.str().c_str());
  }
}

class ObjectRegistry
{
  public:
    static void register_all(const Glib::RefPtr<Gio::DBus::Connection>& connection);
    static void unregister_all();
};

} // saftlib

#endif
