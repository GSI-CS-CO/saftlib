#define ETHERBONE_THROWS 1

#include "SAFTd.h"
#include "Device.h"

// Define all static members in one file to ensure initialization order
namespace saftlib {

Device::irqMap Device::irqs;
Device::msiQueue Device::msis;
SAFTd SAFTd::saftd;

}
