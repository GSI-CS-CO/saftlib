#ifndef TR_SAFTD_HPP_
#define TR_SAFTD_HPP_

#include <saftbus/service.hpp>

#include <memory>
#include <string>
#include <map>

#include "TimingReceiver.hpp"

namespace eb_plugin {

	class SAFTd  : public etherbone::Handler {
	public:
		SAFTd(saftbus::Container *c, const std::string &obj_path);
		~SAFTd();
		// @saftbus-export
		std::string AttachDevice(const std::string& name, const std::string& path);
		// @saftbus-export
		std::string EbForward(const std::string& saftlib_device);
		// @saftbus-export
		void RemoveDevice(const std::string& name);
		// @saftbus-export
		void Quit();
		// @saftbus-export
		std::string getSourceVersion() const;
		// @saftbus-export
		std::string getBuildInfo() const;
		// @saftbus-export
		std::map< std::string, std::string > getDevices() const;
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

		// remember all attached devices. devs.first contains what was given as name argument in AttachDevice-function
		std::map< std::string, TimingReceiver* > devs;
	};


}

#endif

