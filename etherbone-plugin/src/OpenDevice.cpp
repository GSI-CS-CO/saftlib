#include "OpenDevice.hpp"

#include <iostream>

namespace eb_plugin {

OpenDevice::OpenDevice(const etherbone::Socket &socket, const std::string& eb_path)
	: etherbone_path(eb_path)
{
	std::cerr << "OpenDevice::OpenDevice(\"" << eb_path << "\")" << std::endl;
	device.open(socket, etherbone_path.c_str());
	stat(etherbone_path.c_str(), &dev_stat);
}
OpenDevice::~OpenDevice()
{
	chmod(etherbone_path.c_str(), dev_stat.st_mode);
	device.close();
}

std::string OpenDevice::getEtherbonePath() const
{
	return etherbone_path;
}


} // namespace
