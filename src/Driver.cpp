#include "Driver.h"

namespace saftlib {

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

void Drivers::start()
{
  for (std::list<DriverBase*>::const_iterator i = driver_set.begin(); i != driver_set.end(); ++i)
    (*i)->start();
}

void Drivers::stop()
{
  for (std::list<DriverBase*>::const_iterator i = driver_set.begin(); i != driver_set.end(); ++i)
    (*i)->start();
}

}
