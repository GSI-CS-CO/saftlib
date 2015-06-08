#ifndef SAFTLIB_REGISTERED_OBJECT_H
#define SAFTLIB_REGISTERED_OBJECT_H

#include <giomm.h>
#include <etherbone.h>
#include "Server.h"

namespace saftlib {

template <typename I, typename P>
class RegisteredObject : public I
{
  public:
    RegisteredObject(const Glib::ustring& object_path);
    
    const Glib::ustring& getObjectPath();
    const Glib::RefPtr<Gio::DBus::Connection>& getConnection();
    void rethrow(const char *method);
  
  protected:
    P export;
};

template <typename T>
RegisteredObject<T>::RegisteredObject(const Glib::ustring& object_path)
{
  export.register_self(Server::get()->connection(), object_path);
}

template <typename T>
const Glib::ustring& RegisteredObject<T>::getObjectPath()
{ 
  return T::exports[0].object_path;
}

template <typename T>
const Glib::RefPtr<Gio::DBus::Connection>& RegisteredObject<T>::getConnection()
{
  return T::exports[0].connection;
}

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

} // saftlib

#endif
