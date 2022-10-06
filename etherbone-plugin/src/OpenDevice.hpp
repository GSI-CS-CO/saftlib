#ifndef EB_PLUGIN_OPEN_DEVICE_HPP_
#define EB_PLUGIN_OPEN_DEVICE_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <memory>

#include <sys/stat.h>

namespace eb_plugin {

class SAFTd;
class Mailbox;
/// @brief etherbone::Device that is opened on construction and closed before destruction
///
/// It remembers its etherbone path and restores the device settings before destruction.
/// It also checks on startup if MSI needs to be polled from the etherbone-slave config regisers
/// or if the hardware has the capability to deliver real MSIs
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
	// polling for MSIs on hardware that doesn't support real MSIs
	bool poll_msi(bool only_once);
	int polling_interval_ms;

	// following members are for testing MSI capability (real or polled MSIs)
	// The procedure is:
	//  - Register a MSI callback function for a specific address
	//  - Use the Mailbox device to create an MSI with that specific address
	//  - start the periodic polling function
	//  - if the polling function is called and finds the specific MSI, it continues to poll
	//  - if the MSI callback function is called and the polling function didn't see the MSI before, the polling function will be removed from the event loop
	void check_msi_callback(eb_data_t value);
	std::unique_ptr<Mailbox> mbox; // the Mailbox is used to create an MSI
	int slot_idx;         // the Mailbox slot needs to be stored to free the Mailbox slot after MSI type checking is done
	SAFTd *saftd;         // a pointer to SAFTd is needed because in case of polled MSIs the OpenDevice needs to call SAFTds write function
	eb_address_t first, last, mask; // range of addresses that are valid for MSI
	eb_address_t msi_first;         // address offset which needs to be subtracted 
	eb_address_t irq_adr; // the MSI callback function is registered under this address, and the Mailbox is configured with irq_adr+msi_first
	bool check_msi_phase, needs_polling; // check_msi_phase is true until the MSI type was determined.
	                                     // needs_polling is false in the beginning. It is set to true if the poll_msi function receives the injected MSI.
};


} // namespace 

#endif
