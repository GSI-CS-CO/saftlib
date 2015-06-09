#define ETHERBONE_THROWS 1

#include "OpenDevice.h"

namespace saftlib {

OpenDevice::OpenDevice(etherbone::Device d) : device(d) { 
}

}
