#ifndef SAFTLIB_REGISTERED_OBJECT_H
#define SAFTLIB_REGISTERED_OBJECT_H

#include <giomm.h>
#include <etherbone.h>
#include "Directory.h"

namespace saftlib {

template <typename T>
class RegisteredObject : public T
{
  public:
    RegisteredObject(const Glib::ustring& object_path);
    
    const Glib::ustring& getObjectPath();
    void rethrow(const char *method) const;
  
  protected:
    Glib::ustring path;
};

template <typename T>
RegisteredObject<T>::RegisteredObject(const Glib::ustring& object_path)
 : path(object_path)
{
  T::register_self(Directory::get()->connection(), object_path);
}

template <typename T>
const Glib::ustring& RegisteredObject<T>::getObjectPath()
{ 
  return path;
}

template <typename T>
void RegisteredObject<T>::rethrow(const char *method) const
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
