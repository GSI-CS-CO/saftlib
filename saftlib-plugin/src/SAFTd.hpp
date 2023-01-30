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

#ifndef TR_SAFTD_HPP_
#define TR_SAFTD_HPP_

#include <saftbus/service.hpp>

#include <memory>
#include <string>
#include <functional>
#include <map>

#include "TimingReceiver.hpp"
#include "eb-forward.hpp"
#include "MsiDevice.hpp"

namespace saftlib {

	class SAFTd : public etherbone::Handler {
		// @saftbus-default-object-path /de/gsi/saftlib
	public:
		SAFTd(saftbus::Container *container = nullptr);
		~SAFTd();

		/// @brief Instruct saftd to control a new device.
		/// @param name  The logical name for the device
		/// @param path  The etherbone path where the device can be found
		/// @param polling_interval_ms Is the MSI polling interval in 
		///                            milliseconds which is only relevant for 
		///                            devices that have no native MSI support
		/// @return      Object path of the created device
		///
		/// Devices are attached to saftlib by specifying a name and a path.  The
		/// name should denote the logical relationship of the device to saftd. 
		/// For example, baseboard would be a good name for the timing receiver
		/// attached to an SCU.  If an exploder is being used to output events to
		/// an oscilloscope, a good logical name might be scope.  In these
		/// examples, the path for the SCU baseboard would be dev/wbm0, and the
		/// scope exploder would be dev/ttyUSB3 or similar.
		/// This scheme is intended to make it easy to hot-swap hardware. If the
		/// exploder dies, you can simply attach a new one under the same logical
		/// name, even though the path might be different.		
		///
		// @saftbus-export
		std::string AttachDevice(const std::string& name, const std::string& path, int polling_interval_ms = 1);

		/// @brief Remove the device from saftlib management.
		///
		/// @param name        The logical name for the device	
		///
		// @saftbus-export
		void RemoveDevice(const std::string& name);

		/// @brief Instructs the saftlib daemon to quit.
		///
		/// Be absolutely certain before calling this method!
		/// All clients will have their future calls throw exceptions.
		///
		// @saftbus-export
		void Quit();

		/// @brief SAFTd source version.
		///
		/// The version of the SAFTd source code this daemon was compiled from.
		/// Format is "saftlib #.#.# (git-id): MMM DD YYYY HH:MM:SS".		
		///
		// @saftbus-export
		std::string getSourceVersion() const;

		/// @brief  SAFTd build information.
		/// @return  SAFTd build information.
		///
		/// Information about when and where the SAFTd was compiled.
		/// Format is "built by USERNAME on MMM DD YYYY HH:MM:SS with HOSTNAME running OPERATING-SYSTEM".
		///
		// @saftbus-export
		std::string getBuildInfo() const;


		/// @brief List of all devices attached to saftd.
		/// @return List of all devices attached to saftd.
		///
		/// The key is the name of the device as provided to AttachDevice.
		/// The value is the dbus path to the Device object, NOT the etherbone
		/// path of the device. Each object is guaranteed to implement at least
		/// the Device interface, however, typically the objects implement the
		/// TimingReceiver interface.
		///
		// @saftbus-export
		std::map< std::string, std::string > getDevices() const;

		/// @brief Get the name of the device that forwards etherbone requests to saftlib_device.
		///
		/// @param saftlib_device The name of an attached device.
		/// @param return The name any eb-tool can use to communicate with the hardware attached to saftlib_device.
		///
		// @saftbus-export
		std::string EbForward(const std::string& saftlib_device);

		/// @brief release a callback
		/// @param irq the address to be released
		void release_irq(eb_address_t irq);

		/// @brief try to attach a callback to an irq address
		///
		/// @param object of type MsiDevice or derived from MsiDevice. MsiDevices are masters on the MSI-crossbar interconnect
		/// @param slot function object that is called when an MSI with the correct address (return value of this fuction) arrives
		/// @return the address that is associated with the function object connected to slot.
		eb_address_t request_irq(MsiDevice &msi, const std::function<void(eb_data_t)>& slot);

		std::string getObjectPath();

		etherbone::Socket &get_etherbone_socket() { return socket; }

		TimingReceiver* getTimingReceiver(const std::string &object_path);

		// Override the virtual functions from etherbone::Handler base class
		// to receive incoming etherbone read/write requests from the device.
		// Only write is ever used (an incoming MSI causes a write request).
		eb_status_t read (eb_address_t address, eb_width_t width, eb_data_t* data);
		eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);

	private:


		/// @brief try to attach a callback to an irq address
		///
		/// @param irq the address at which the callback should be attached
		/// @param slot the function object
		/// @return true if the requested address was still free, false if the 
		///         requested address was already in use
		bool request_irq(eb_address_t irq, const std::function<void(eb_data_t)>& slot);

		void RemoveObject(const std::string& name);

		// The sdb structure for this "virtual" etherbone device
		sdb_device eb_slave_sdb;


		// Need a pointer to saftbus::Container to insert new Service Objects (instances of TimingReceiver_Service)
		// and the object_path of the SAFTd_Service (normally "/de/gsi/saftlib") as prefix for the object_path of TimingReceiver_Service objects
		saftbus::Container *container;
		std::string object_path;

		// 
		etherbone::Socket socket;
		saftbus::SourceHandle eb_source;

		std::map< std::string, std::unique_ptr<EB_Forward> > eb_forward; 

		// remember all attached devices 
		std::map<std::string, std::unique_ptr<TimingReceiver> > attached_devices;

		std::map<eb_address_t, std::function<void(eb_data_t)> > irqs;

	};


}

#endif

