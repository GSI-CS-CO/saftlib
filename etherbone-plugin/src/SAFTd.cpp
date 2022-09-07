
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


	SAFTd::SAFTd(saftbus::Container *c, const std::string &obj_path) 
		: container(c)
		, object_path(obj_path)
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
		for (auto &dev: devs) {
			RemoveDevice(dev.first);
		}
		saftbus::Loop::get_default().remove(eb_source);
		socket.close();
	}

	eb_status_t SAFTd::read(eb_address_t address, eb_width_t width, eb_data_t* data) {
		*data = 0;
		return EB_OK;
	}

	eb_status_t SAFTd::write(eb_address_t address, eb_width_t width, eb_data_t data) {
		std::cerr << "write callback " << address << " " << data << std::endl;
	    
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
		if (devs.find(name) != devs.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "device already exists");
		}
		if (find_if(name.begin(), name.end(), [](char c){ return !(isalnum(c) || c == '_');} ) != name.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
		}

		try {
			// create the TimingReceiver and keep a bare pointer to it (for later use)
			std::unique_ptr<TimingReceiver> instance(new TimingReceiver(container, socket, object_path, name, etherbone_path));
			TimingReceiver *timing_receiver = instance.get();
			devs.insert(std::make_pair(name, timing_receiver));

			// crate a TimingReceiver_Service object
			std::unique_ptr<TimingReceiver_Service> service (new TimingReceiver_Service(std::move(instance)));

			// insert the Service object
			container->create_object(timing_receiver->get_object_path(), std::move(service));

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

	}
	void SAFTd::Quit() {

	}
	std::string SAFTd::getSourceVersion() const {
		return std::string();

	}
	std::string SAFTd::getBuildInfo() const {
		return std::string();

	}
	std::map< std::string, std::string > SAFTd::getDevices() const {
		std::map<std::string, std::string> result;
		for (auto &dev: devs) {
			result.insert(std::make_pair(dev.first, dev.second->getEtherbonePath()));
		}
		return result;
	}


	void SAFTd::request_irq(eb_address_t irq, const std::function<void(eb_data_t)>& slot) 
	{
		irqs[irq] = slot;
	}
	void SAFTd::release_irq(eb_address_t irq) {
		irqs.erase(irq);
	}


}