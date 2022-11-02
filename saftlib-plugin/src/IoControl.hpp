#ifndef saftlib_IO_CONTROL_HPP_
#define saftlib_IO_CONTROL_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include "Io.hpp"
#include "SerdesClockGen.hpp"

namespace saftlib {

class IoControl {
	etherbone::Device &device;

	SerdesClockGen clkgen;

	std::vector<Io> ios;
public:
	IoControl(etherbone::Device &device);

	std::vector<Io> & get_ios();
};

}

#endif
