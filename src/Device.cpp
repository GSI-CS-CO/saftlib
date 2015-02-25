#include "Device.h"

namespace saftlib {

Device::Device(etherbone::Device d, eb_address_t low_, eb_address_t high_)
 : etherbone::Device(d)
{
  low  = low_;
  high = high_;
}

} // saftlib
