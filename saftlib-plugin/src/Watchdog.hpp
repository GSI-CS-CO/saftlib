#ifndef EB_PLUGIN_WATCHDOG_HPP_
#define EB_PLUGIN_WATCHDOG_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

namespace eb_plugin {

class Watchdog {
	etherbone::Device &device;
	eb_address_t watchdog;
	eb_data_t watchdog_value;
public:
	Watchdog(etherbone::Device &device);
	bool aquire();
	void update();
};

}

#endif
