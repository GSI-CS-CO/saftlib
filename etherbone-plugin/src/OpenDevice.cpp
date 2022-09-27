#include "OpenDevice.hpp"

#include <iostream>

namespace eb_plugin {

OpenDevice::OpenDevice(const etherbone::Socket &socket, const std::string& eb_path)
	: etherbone_path(eb_path)
{
	std::cerr << "OpenDevice::OpenDevice()" << std::endl;
	stat(etherbone_path.c_str(), &dev_stat);
	device.open(socket, etherbone_path.c_str());
}
OpenDevice::~OpenDevice()
{
	device.close();
	chmod(etherbone_path.c_str(), dev_stat.st_mode);
}

} // namespace
