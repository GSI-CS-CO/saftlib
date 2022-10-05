#ifndef EB_PLUGIN_OPEN_DEVICE_HPP_
#define EB_PLUGIN_OPEN_DEVICE_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <saftbus/loop.hpp>

#include <memory>

#include <sys/stat.h>

namespace eb_plugin {

class SAFTd;
class Mailbox;
/// @brief etherbone::Device that is opened on construction and closed before destruction
///
/// It also remembers its etherbone path and restores the device settings before destruction
class OpenDevice {
protected:
	std::string etherbone_path;
	struct stat dev_stat;	
	mutable etherbone::Device device;

public:
	OpenDevice(const etherbone::Socket &socket, const std::string& etherbone_path, int polling_interval_ms = 1, SAFTd *saftd = nullptr);
	virtual ~OpenDevice();


	/// @brief The path through which the device is reached.
	/// @return The path through which the device is reached.
	///
	// @saftbus-export
	std::string getEtherbonePath() const;




private:
	void check_msi_callback(eb_data_t value);
	bool poll_msi();
	int polling_interval_ms;
	// following members are only used for testing MSI capability (real or polled MSIs)
	std::unique_ptr<Mailbox> mbox;
	SAFTd *saftd;
	eb_address_t irq_adr; 
	int slot_idx;        
	eb_address_t first, last, mask;
	eb_address_t msi_first;
	bool check_msi_type, needs_polling;
	saftbus::Source *poll_timeout_source;

};


} // namespace 

#endif
