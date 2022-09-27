#ifndef EB_PLUGIN_WATCHDOG_HPP_
#define EB_PLUGIN_WATCHDOG_HPP_

#include "OpenDevice.hpp"

namespace eb_plugin {

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

class Watchdog : public OpenDevice {
	eb_address_t watchdog;
	eb_data_t watchdog_value;
public:
	Watchdog(const etherbone::Socket &socket, const std::string& etherbone_path);
	bool aquire();
	void update();
};

}

#endif
