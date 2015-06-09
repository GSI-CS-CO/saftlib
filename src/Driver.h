#ifndef SAFTLIB_DRIVER_H
#define SAFTLIB_DRIVER_H

#include "OpenDevice.h"

namespace saftlib {

class DriverBase
{
  public:
    DriverBase() { }
    void insert_self();
    void remove_self();
    
  protected:
    virtual void probe(OpenDevice& od) = 0;
    
  private:
    DriverBase *next;
    
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
    void probe(OpenDevice& od) { return T::probe(od); }
};

class Drivers
{
  public:
    static void probe(OpenDevice& od);
};

}

#endif
