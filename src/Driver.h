#ifndef SAFTLIB_DRIVER_H
#define SAFTLIB_DRIVER_H

#include <list>
#include "Device.h"

namespace saftlib {

extern Gio::DBus::Error eb_error(const char *method, const etherbone::exception_t& e);

class DriverBase
{
  public:
    DriverBase() { }
    void insert_self();
    void remove_self();
    
  protected:
    virtual void start(Devices&) = 0;
    virtual void stop() = 0;
    
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
    void start(Devices& devices) { new T(devices); }
    void stop() { }
};

class Drivers
{
  public:
    static void start(Devices& devices);
    static void stop();
};

}

#endif
