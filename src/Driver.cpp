#define ETHERBONE_THROWS 1

#include <sstream>
#include "Driver.h"

namespace saftlib {

Gio::DBus::Error eb_error(const char *method, const etherbone::exception_t& e) {
  std::ostringstream str;
  str << method << ": " << e;
  return Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, str.str().c_str());
}

// !!! dangerous because the order of static initialization might leave this
// uninitialized at the time of .insert_self
static std::list<DriverBase*> driver_set;

void DriverBase::insert_self()
{
  index = driver_set.insert(driver_set.begin(), this);
}

void DriverBase::remove_self()
{
  driver_set.erase(index);
}

void Drivers::start(Devices& devices)
{
  for (std::list<DriverBase*>::const_iterator i = driver_set.begin(); i != driver_set.end(); ++i)
    (*i)->start(devices);
}

void Drivers::stop()
{
  for (std::list<DriverBase*>::const_iterator i = driver_set.begin(); i != driver_set.end(); ++i)
    (*i)->stop();
}

}
