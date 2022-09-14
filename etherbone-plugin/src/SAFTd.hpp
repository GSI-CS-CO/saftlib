#ifndef TR_SAFTD_HPP_
#define TR_SAFTD_HPP_

#include <saftbus/service.hpp>

#include <memory>
#include <string>
#include <functional>
#include <map>

#include "TimingReceiver.hpp"

namespace eb_plugin {

	class SAFTd : public etherbone::Handler {
	public:
		SAFTd(saftbus::Container *container = nullptr);
		~SAFTd();

		/// @brief Instruct saftd to control a new device.
		/// @param name  The logical name for the device
		/// @param path  The etherbone path where the device can be found
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
		std::string AttachDevice(const std::string& name, const std::string& path);

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

		void request_irq(eb_address_t irq, const std::function<void(eb_data_t)>& slot);
		void release_irq(eb_address_t irq);

		std::string get_object_path();

	private:

		// The sdb structure for this "virtual" etherbone device
		sdb_device eb_slave_sdb;

		// Override the virtual functions from etherbone::Handler base class
		// to receive incoming etherbone read/write requests from the device.
		// Only write is ever used (an incoming MSI causes a write request).
		eb_status_t read (eb_address_t address, eb_width_t width, eb_data_t* data);
		eb_status_t write(eb_address_t address, eb_width_t width, eb_data_t data);

		// Need a pointer to saftbus::Container to insert new Service Objects (instances of TimingReceiver_Service)
		// and the object_path of the SAFTd_Service (normally "/de/gsi/saftlib") as prefix for the object_path of TimingReceiver_Service objects
		saftbus::Container *container;
		std::string object_path;

		// 
		etherbone::Socket socket;
		saftbus::Source *eb_source;

		// remember all attached devices 
		std::map<std::string, std::unique_ptr<TimingReceiver> > attached_devices;

		std::map<eb_address_t, std::function<void(eb_data_t)> > irqs;
	};


}

#endif

