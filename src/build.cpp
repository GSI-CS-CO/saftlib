#define ETHERBONE_THROWS 1

#include "config.h"
#include "version.h"
#include "build.h"

namespace saftlib {

const char sourceVersion[] =
  PACKAGE_STRING " (" GIT_ID "): " SOURCE_DATE;
const char buildInfo[] =
  "built by " USERNAME " on " __DATE__ " " __TIME__ " with " HOSTNAME " running " OPERATING_SYSTEM;

}
