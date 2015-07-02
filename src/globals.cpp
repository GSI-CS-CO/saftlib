#define ETHERBONE_THROWS 1

#include "SAFTd.h"
#include "Device.h"
#include "clog.h"

// Define all static members in one file to ensure initialization order
namespace saftlib {

CLogStream clog("saftd", LOG_DAEMON);
Device::irqMap Device::irqs;
Device::msiQueue Device::msis;
SAFTd SAFTd::saftd;

}
