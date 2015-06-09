#ifndef SAFTLIB_OPEN_DEVICE_H
#define SAFTLIB_OPEN_DEVICE_H

#include "Device.h"

namespace saftlib {

struct OpenDevice {
  Device device;
  Glib::ustring path;
  Glib::RefPtr<Glib::Object> ref;
  
  OpenDevice(etherbone::Device d);
};

}

#endif
