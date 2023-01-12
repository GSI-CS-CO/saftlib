/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#ifndef saftlib_OPEN_DEVICE_HPP_
#define saftlib_OPEN_DEVICE_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <saftbus/loop.hpp>

#include <memory>

#include <sys/stat.h>

namespace saftlib {

class SAFTd;
class Mailbox;
class EB_Forward;
/// @brief Holds etherbone::Device that is opened on construction and closed on destruction
///
/// It remembers its etherbone path and restores the device settings before destruction.
/// It also checks on startup if MSI needs to be polled from the etherbone-slave config regisers
/// or if the hardware has the capability to deliver MSIs without polling
/// The checking procedure for MSI type is:
///  - Register a MSI callback function for a specific address
///  - Use the Mailbox device to send an MSI value with that specific address
///  - start the periodic polling function
///  - if the polling function is called and finds the specific MSI value, it continues to poll
///  - if the MSI callback function is called despite of the polling function not seeing the MSI value, the polling function will be removed from the event loop
///
class OpenDevice {
protected:
	std::string etherbone_path;
	struct stat dev_stat;	
	etherbone::Device device;

public:
	OpenDevice(const etherbone::Socket &socket, const std::string& etherbone_path, int polling_interval_ms = 1, SAFTd *saftd = nullptr);
	virtual ~OpenDevice();


	/// @brief The path through which the device is reached.
	/// @return The path through which the device is reached.
	///
	// @saftbus-export
	std::string getEtherbonePath() const;


	/// @brief If the device is not capable of multiplexing multiple users
	///        a /dev/pts/<num> device is created that can be used with eb-tools
	/// @return The path which can be used by eb-tools to access the device.
	///         If the etherbone device has multiplexing capability no forwarding device 
	///         is created and this function returns the original etherbone path of the device
	///
	// @saftbus-export
	std::string getEbForwardPath() const;

private:
	// etherbone forwading
	std::unique_ptr<EB_Forward> eb_forward;
	std::string eb_forward_path;

	// polling for MSIs on hardware that doesn't support real MSIs
	bool poll_msi(bool only_once);
	int polling_interval_ms;
	saftbus::SourceHandle poll_timeout_source;

	// following members are for testing MSI capability (real or polled MSIs)
	void check_msi_callback(eb_data_t value);
	SAFTd *saftd;         // a pointer to SAFTd is needed because in case of polled MSIs the OpenDevice needs to call SAFTds write function
	eb_address_t first, last, mask; // range of addresses that are valid for MSI
	eb_address_t msi_first, msi_last;         // address offset which needs to be subtracted 
	eb_address_t irq_adr; // the MSI callback function is registered under this address, and the Mailbox is configured with irq_adr+msi_first
	bool check_msi_phase, needs_polling; // check_msi_phase is true until the MSI type was determined.
	                                     // needs_polling is false in the beginning. It is set to true if the poll_msi function receives the injected MSI.
};


} // namespace 

#endif
