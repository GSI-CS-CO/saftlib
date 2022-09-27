#ifndef EB_PLUGIN_OPEN_DEVICE_HPP_
#define EB_PLUGIN_OPEN_DEVICE_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <sys/stat.h>

namespace eb_plugin {

/// @brief etherbone::Device that is opened on construction and closed before destruction
///
/// It also remembers its etherbone path and restores the device settings before destruction
class OpenDevice {
protected:
	std::string etherbone_path;
	struct stat dev_stat;	
	mutable etherbone::Device device;
public:
	OpenDevice(const etherbone::Socket &socket, const std::string& eb_path);
	virtual ~OpenDevice();
};


} // namespace 

#endif
