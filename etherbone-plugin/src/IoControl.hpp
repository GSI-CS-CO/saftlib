#ifndef EB_PLUGIN_IO_CONTROL_HPP_
#define EB_PLUGIN_IO_CONTROL_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include "Io.hpp"

namespace eb_plugin {

class IoControl {
	etherbone::Device &device;

	std::vector<Io> ios;
public:
	IoControl(etherbone::Device &device);
};

}

#endif
