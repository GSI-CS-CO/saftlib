#ifndef EB_PLUGIN_WATCHDOG_HPP_
#define EB_PLUGIN_WATCHDOG_HPP_

#include "WhiteRabbit.hpp"

namespace eb_plugin {

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

class Watchdog : public WhiteRabbit {
	eb_address_t watchdog;
	eb_data_t watchdog_value;
public:
	Watchdog(const etherbone::Socket &socket, const std::string& etherbone_path);
	bool aquire();
	void update();
};

}

#endif
