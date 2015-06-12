#ifndef SAFTLIB_REGISTERED_OBJECT_H
#define SAFTLIB_REGISTERED_OBJECT_H

#include <giomm.h>
#include <etherbone.h>
#include "SAFTd.h"

namespace saftlib {

// T must be derived from Glib::Object
// T must provide T::ServiceType     -> an interface Service
// T must provide T::ConstructorType -> the type used for its constructor
template <typename T>
class RegisteredObject : public T
{
  public:
    static Glib::RefPtr< RegisteredObject<T> > create(const Glib::ustring& object_path, typename T::ConstructorType args);
    
    const Glib::RefPtr<Gio::DBus::Connection>& getConnection() const;
    const Glib::ustring& getObjectPath() const;
    const Glib::ustring& getSender() const;
    
  protected:
    RegisteredObject(const Glib::ustring& object_path, typename T::ConstructorType args);
    virtual void rethrow(const char *method) const;
    
    typename T::ServiceType service;
    Glib::ustring path;
};

template <typename T>
Glib::RefPtr< RegisteredObject<T> > RegisteredObject<T>::create(const Glib::ustring& object_path, typename T::ConstructorType args)
{
  return Glib::RefPtr< RegisteredObject<T> >(new RegisteredObject<T>(object_path, args));
}

template <typename T>
RegisteredObject<T>::RegisteredObject(const Glib::ustring& object_path, typename T::ConstructorType args)
 : T(args), service(this, sigc::mem_fun(this, &RegisteredObject<T>::rethrow)), path(object_path)
{
  service.register_self(SAFTd::get()->connection(), object_path);
}

template <typename T>
const Glib::RefPtr<Gio::DBus::Connection>& RegisteredObject<T>::getConnection() const
{
  return service.getConnection();
}

template <typename T>
const Glib::ustring& RegisteredObject<T>::getObjectPath() const
{ 
  return path;
}

template <typename T>
const Glib::ustring& RegisteredObject<T>::getSender() const
{
  return service.getSender();
}

template <typename T>
void RegisteredObject<T>::rethrow(const char *method) const
{
  try {
    throw;
  } catch (const etherbone::exception_t& e) {
    std::ostringstream str;
    str << method << ": " << e;
    throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, str.str().c_str());
  }
}

} // saftlib

#endif
