#ifndef EB_PLUGIN_BUILD_ID_ROM_HPP_
#define EB_PLUGIN_BUILD_ID_ROM_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>


#include <map>
#include <string>

namespace eb_plugin {

class BuildIdRom {
	etherbone::Device &device;
	eb_address_t info;

	std::map<std::string, std::string> gateware_info;
	void setupGatewareInfo(uint32_t address);
public:
	BuildIdRom(etherbone::Device &device);

	/// @brief Key-value map of hardware build information
	/// @return Key-value map of hardware build information
	///
	// @saftbus-export
	std::map< std::string, std::string > getGatewareInfo() const;

	/// @brief Hardware build version
	/// @return "major.minor.tiny" if version is valid (or "N/A" if not available)
	///
	// @saftbus-export
	std::string getGatewareVersion() const;

};


}


#endif
