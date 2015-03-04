#ifndef SAFTLIB_DRIVER_H
#define SAFTLIB_DRIVER_H

#include <list>
#include "Device.h"

namespace saftlib {

class DriverBase
{
  public:
    DriverBase() { }
    void insert_self();
    void remove_self();
    
  protected:
    virtual void probe() = 0;
    
  private:
    // secret
    std::list<DriverBase*>::iterator index;
    
    // non-copyable
    DriverBase(const DriverBase&);
    DriverBase& operator = (const DriverBase&);
  
  friend class Drivers;
};

template <typename T>
class Driver : private DriverBase
{
  public:
    Driver() { insert_self(); }
    ~Driver() { remove_self(); }
  
  private:
    void probe() { return T::probe(); }
};

class Drivers
{
  public:
    static void probe();
};

}

#endif
