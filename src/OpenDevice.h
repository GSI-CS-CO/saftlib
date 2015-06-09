#ifndef SAFTLIB_OPEN_DEVICE_H
#define SAFTLIB_OPEN_DEVICE_H

#include "Device.h"

namespace saftlib {

struct OpenDevice {
  Device dev;
  Glib::RefPtr<Glib::Object> ref;
  Glib::ustring path;
  
  OpenDevice(etherbone::Device d);
  ~OpenDevice();
};

}

#endif
