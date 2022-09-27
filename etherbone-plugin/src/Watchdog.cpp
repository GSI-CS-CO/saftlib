#include "Watchdog.hpp"

#include <saftbus/error.hpp>

#include <iostream>

#define WATCHDOG_VENDOR_ID        0x00000651
#define WATCHDOG_DEVICE_ID        0xb6232cd3

namespace eb_plugin {

Watchdog::Watchdog(const etherbone::Socket &socket, const std::string& etherbone_path) 
	: OpenDevice(socket, etherbone_path)
{
	std::vector<sdb_device> watchdogs_dev;
	device.sdb_find_by_identity(WATCHDOG_VENDOR_ID, WATCHDOG_DEVICE_ID, watchdogs_dev);
	if (watchdogs_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no watchdog device found on hardware");
	}
	if (watchdogs_dev.size() > 1) {
		std::cerr << "more than one watchdog device found on hardware, taking the first one" << std::endl;
	}
	watchdog = static_cast<eb_address_t>(watchdogs_dev[0].sdb_component.addr_first);
	if (!aquire()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Watchdog on Timing Receiver already locked");
	}
}

bool Watchdog::aquire() {
	// try to acquire watchdog
	eb_data_t retry;
	device.read(watchdog, EB_DATA32, &watchdog_value);
	if ((watchdog_value & 0xFFFF) != 0) {
		return false;
	}
	device.write(watchdog, EB_DATA32, watchdog_value);
	device.read(watchdog, EB_DATA32, &retry);
	if (((retry ^ watchdog_value) >> 16) != 0) {
		return false;
	}
	return true;
}

void Watchdog::update() {
	device.write(watchdog, EB_DATA32, watchdog_value);
}

} // namespace