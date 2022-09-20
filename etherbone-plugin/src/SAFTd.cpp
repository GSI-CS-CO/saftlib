
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>

#include <saftbus/error.hpp>
#include <saftbus/loop.hpp>

#include "eb-source.hpp"

#include "SAFTd.hpp"
#include "SAFTd_Service.hpp"

#include "TimingReceiver.hpp"
#include "TimingReceiver_Service.hpp"


namespace eb_plugin {


	SAFTd::SAFTd(saftbus::Container *cont)
		: container(cont)
		, object_path("/de/gsi/saftlib")
	{
		socket.open();

		eb_slave_sdb.abi_class     = 0;
		eb_slave_sdb.abi_ver_major = 0;
		eb_slave_sdb.abi_ver_minor = 0;
		eb_slave_sdb.bus_specific  = SDB_WISHBONE_WIDTH;
		eb_slave_sdb.sdb_component.addr_first = 0;
		eb_slave_sdb.sdb_component.addr_last  = UINT32_C(0xffffffff);
		eb_slave_sdb.sdb_component.product.vendor_id = 0x651;
		eb_slave_sdb.sdb_component.product.device_id = 0xefaa70;
		eb_slave_sdb.sdb_component.product.version   = 1;
		eb_slave_sdb.sdb_component.product.date      = 0x20150225;
		memcpy(eb_slave_sdb.sdb_component.product.name, "SAFTLIB           ", 19);
		socket.attach(&eb_slave_sdb, this);

		// connect the eb-source to saftbus::Loop in order to react on incoming MSIs from hardware
		eb_source = saftbus::Loop::get_default().connect<eb_plugin::EB_Source>(socket);
	}

	SAFTd::~SAFTd() 
	{
		std::cerr << "~SAFTd()" << std::endl;
		if (container) {
			for (auto &device: attached_devices) {
				std::cerr << "  remove " << device.second->get_object_path() << std::endl;
				container->remove_object_delayed(device.second->get_object_path());
			}
		}
		std::cerr << "attached_devices.clear()" << std::endl;
		attached_devices.clear();
		std::cerr << "saftbus::Loop::get_default().remove(eb_source);" << std::endl;
		saftbus::Loop::get_default().remove(eb_source);
		std::cerr << "socket.close()" << std::endl;
		socket.close();
	}

	eb_status_t SAFTd::read(eb_address_t address, eb_width_t width, eb_data_t* data) {
		*data = 0;
		return EB_OK;
	}

	eb_status_t SAFTd::write(eb_address_t address, eb_width_t width, eb_data_t data) {
		std::cerr << "write callback " << std::hex << std::setw(8) << std::setfill('0') << address 
		          <<               " " << std::hex << std::setw(8) << std::setfill('0') << data 
		          << std::dec 
		          << std::endl;
	    
	    std::map<eb_address_t, std::function<void(eb_data_t)> >::iterator it = irqs.find(address);
	    if (it != irqs.end()) {
	      try {
	        it->second(data);
	      } catch (...) {
	        std::cerr << "Unhandled unknown exception in MSI handler for 0x" 
	             << std::hex << address << std::dec << std::endl;
	      }
	    } else {
	      std::cerr << "No handler for MSI 0x" << std::hex << address << std::dec << std::endl;
	    }

		return EB_OK;
	}

	std::string SAFTd::AttachDevice(const std::string& name, const std::string& etherbone_path) 
	{
		if (attached_devices.find(name) != attached_devices.end()) {
	        throw saftbus::Error(saftbus::Error::INVALID_ARGS, "device already exists");
		}
		try {
			// create a new TimingReceiver object and add it to the attached_devices
			TimingReceiver *timing_receiver = new TimingReceiver(this, name, etherbone_path, container);
			attached_devices[name] = std::move(std::unique_ptr<TimingReceiver>(timing_receiver));

			// crate a TimingReceiver_Service object
			if (container) {
				std::unique_ptr<TimingReceiver_Service> service (new TimingReceiver_Service(timing_receiver));

				// insert the Service object
				container->create_object(timing_receiver->get_object_path(), std::move(service));
			}

			// return the object path to the new Servie object
			return timing_receiver->get_object_path();

		} catch (const etherbone::exception_t& e) {
			std::ostringstream str;
			str << "AttachDevice: failed to open: " << e;
			throw saftbus::Error(saftbus::Error::IO_ERROR, str.str().c_str());
		}
		return std::string();
	}


	std::string SAFTd::EbForward(const std::string& saftlib_device) {
		return std::string();
	}


	void SAFTd::RemoveDevice(const std::string& name) {
		std::map< std::string, std::unique_ptr<TimingReceiver> >::iterator device = attached_devices.find(name);
		if (device == attached_devices.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such device");
		}
		if (container) {
			container->remove_object_delayed(device->second->get_object_path());
		}
		attached_devices.erase(device);
	}


	void SAFTd::Quit() {
		if (container) {
			container->remove_object(object_path);
		}
	}


	std::string SAFTd::getSourceVersion() const {
		return std::string();
	}

	std::string SAFTd::getBuildInfo() const {
		return std::string();
	}

	std::map< std::string, std::string > SAFTd::getDevices() const {
		std::map<std::string, std::string> result;
		for (auto &device: attached_devices) {
			result.insert(std::make_pair(device.first, device.second->getEtherbonePath()));
		}
		return result;
	}


	bool SAFTd::request_irq(eb_address_t irq, const std::function<void(eb_data_t)>& slot) 
	{
		auto it = irqs.find(irq);
		if (it == irqs.end()) {
			// the requested address is still free
			std::cerr << "attach irq for address 0x" << std::hex << std::setw(8) << std::setfill('0') << irq << std::endl;
			irqs[irq] = slot;
			return true;
		}
		// the requested address is already in use
		return false;
	}
	void SAFTd::release_irq(eb_address_t irq) {
		auto it = irqs.find(irq);
		if (it != irqs.end()) {
			irqs.erase(it);
		}
	}

	std::string SAFTd::get_object_path() {
		return object_path;
	}


	TimingReceiver* SAFTd::getTimingReceiver(const std::string &tr_obj_path) {
		size_t pos;
		std::cerr << "is " << object_path << " contained in " << tr_obj_path << "? " << std::endl;
		if ((pos=tr_obj_path.find(object_path)) == tr_obj_path.npos) {
			std::cerr << "no" << std::endl;
			const std::string &tr_name = tr_obj_path; // maybe the name of the TimingReceiver was given instead of its object_path
			if (attached_devices.find(tr_name) != attached_devices.end()) {
				return attached_devices[tr_name].get();
			} 
		} else {
			std::cerr << "yes" << std::endl;
			std::string name = tr_obj_path.substr(object_path.size()+1);
			std::cerr << "get_timing_receiver " << name << std::endl;
			return attached_devices[name].get();
		}
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such device");
	}

}