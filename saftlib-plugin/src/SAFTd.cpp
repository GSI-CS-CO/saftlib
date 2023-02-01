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

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>

#include <saftbus/error.hpp>
#include <saftbus/loop.hpp>

#include "eb-source.hpp"

#include "SAFTd.hpp"

#include "build.hpp"

#include "TimingReceiver.hpp"
#include "TimingReceiver_Service.hpp"


namespace saftlib {


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
		eb_source = saftbus::Loop::get_default().connect<saftlib::EB_Source>(socket);
	}

	SAFTd::~SAFTd() 
	{
		std::cerr << "~SAFTd()" << std::endl;
		//saftbus::Loop::get_default().remove(eb_source);
		Quit();
		if (container) {
			for (auto &device: attached_devices) {
				// std::cerr << "  remove " << device.second->getObjectPath() << std::endl;
				try {
					container->remove_object(device.second->getObjectPath());
				} catch(...) {
					// nothing
				}
			}
		}
		// std::cerr << "attached_devices.clear()" << std::endl;
		attached_devices.clear();
		// std::cerr << "saftbus::Loop::get_default().remove(eb_source);" << std::endl;
		try {
			// std::cerr << "socket.close()" << std::endl;
			socket.close();
			// std::cerr << "~SAftd done" << std::endl;
		} catch (etherbone::exception_t &e) {
			// std::cerr << "~SAftd exception: " << e << std::endl;
		}
		std::cerr << "~SAFTd() done" << std::endl;		
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

	std::string SAFTd::AttachDevice(const std::string& name, const std::string& etherbone_path, int polling_interval_ms) 
	{
		// std::cerr << etherbone_path << std::endl;
		if (attached_devices.find(name) != attached_devices.end()) {
	        throw saftbus::Error(saftbus::Error::INVALID_ARGS, "device already exists");
		}
		try {
			// create a new TimingReceiver object and add it to the attached_devices
			TimingReceiver *timing_receiver = new TimingReceiver(*this, name, etherbone_path, polling_interval_ms, container);
			attached_devices[name] = std::move(std::unique_ptr<TimingReceiver>(timing_receiver));

			// crate a TimingReceiver_Service object
			if (container) {
				std::unique_ptr<TimingReceiver_Service> service (new TimingReceiver_Service(timing_receiver, std::bind(&SAFTd::RemoveObject, this, name)));

				// insert the Service object
				container->create_object(timing_receiver->getObjectPath(), std::move(service));
			}

			// return the object path to the new Service object
			return timing_receiver->getObjectPath();

		} catch (const etherbone::exception_t& e) {
			std::ostringstream str;
			str << "AttachDevice: failed to open: " << e;
			throw saftbus::Error(saftbus::Error::IO_ERROR, str.str().c_str());
		}
		return std::string();
	}


	std::string SAFTd::EbForward(const std::string& saftlib_device) {
		auto dev = attached_devices.find(saftlib_device);
		if (dev != attached_devices.end()) {
			return dev->second->getEbForwardPath();
		}
		return std::string();
	}


	void SAFTd::RemoveDevice(const std::string& name) {
		// std::cerr << "SAFTd::RemoveDevice(" << name << ")" << std::endl;
		std::map< std::string, std::unique_ptr<TimingReceiver> >::iterator device = attached_devices.find(name);
		if (device == attached_devices.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such device");
		}
		// std::cerr << "SAFTd::RemoveDevice(" << name << ") was found" << std::endl;
		if (container) {
			container->remove_object(device->second->getObjectPath()); // the destruction_callback will call RemoveObject
		} else {
			RemoveObject(name); // if we are not living inside of a saftbus::Container, we call RemoveObject ourselfs
		}
	}
	void SAFTd::RemoveObject(const std::string& name) {
		std::map< std::string, std::unique_ptr<TimingReceiver> >::iterator device_driver = attached_devices.find(name);
		attached_devices.erase(device_driver);
	}


	void SAFTd::Quit() {
		saftbus::Loop::get_default().remove(eb_source);
		if (container) {
			try {
				container->remove_object(object_path);
			} catch (...) {
				// nothing
			}
		}
	}


	std::string SAFTd::getSourceVersion() const {
		return sourceVersion;
	}

	std::string SAFTd::getBuildInfo() const {
		return buildInfo;
	}

	std::map< std::string, std::string > SAFTd::getDevices() const {
		std::map<std::string, std::string> result;
		for (auto &device: attached_devices) {
			result.insert(std::make_pair(device.first, device.second->getObjectPath()));
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
			// std::cerr << "release irq for address 0x" << std::hex << std::setw(8) << std::setfill('0') << irq << std::endl;
			irqs.erase(it);
		}
	}

	// Confirm that first is aligned to size
	// e.g. first = 0x10000  last = 0x1ffff 
	// => size_mask = 0x0ffff
	// => 0x10000 & 0x0ffff = 0
	// eb_address_t size_mask = msi_last - msi_first;
	// msi_first and msi_last is the address range in which the msi_master (on hardware) can reach the host
	// for a host connected with PCIe, this might be msi_first=0x10000 and msi_last=0x1ffff
	// for a host connected with USB (same hardware) msi_first=0x20000 and msi_last=0x2ffff

	// the first and last values obtained from from enable_msi(&first, &last) refers to the address range 
	// available to an etherbone master on a host connected to the pcie-wishbone bridge driver
	// for example, if two processes are connected to dev/wbm0, 
	// the first gets:  first = 0x00000 and last = 0x00fff
	// the second gets: first = 0x01000 and last = 0x01fff
	//
	// if a hardware MSI master writes to 0x10400, the first  etherbone master (connected to dev/wbm0) will get a callback on address 0x400
	// if a hardware MSI master writes to 0x11400, the second etherbone master (connected to dev/wbm0) will get a callback on address 0x400
	// if a hardware MSI master writes to 0x20400, another etherbone master connected via usb will get the MSI on address 0x400
	// if ((msi_first & size_mask) != 0) {
	// 	throw etherbone::exception_t("request_irq/misaligned", EB_FAIL);
	// }


	eb_address_t SAFTd::request_irq(MsiDevice &msi, const std::function<void(eb_data_t)>& slot) {
		eb_address_t first, last, mask;
		msi.device.enable_msi(&first, &last);
		mask = last - first;
		for (;;) {
			eb_address_t irq_adr = ((rand() & mask) + first) & (~0x3);
			if (request_irq(irq_adr, slot)) {
				std::cerr << "msi.msi_device.msi_first = " << std::hex << std::setw(8) << std::setfill('0') << msi.msi_device.msi_first << std::dec << std::endl;
				return msi.msi_device.msi_first + irq_adr; // return the adress that triggers the msi
			}
		}		
	}


	std::string SAFTd::getObjectPath() {
		return object_path;
	}


	TimingReceiver* SAFTd::getTimingReceiver(const std::string &tr_obj_path) {
		size_t pos;
		// std::cerr << "is " << object_path << " contained in " << tr_obj_path << "? " << std::endl;
		if ((pos=tr_obj_path.find(object_path)) == tr_obj_path.npos) {
			// std::cerr << "no" << std::endl;
			const std::string &tr_name = tr_obj_path; // maybe the name of the TimingReceiver was given instead of its object_path
			if (attached_devices.find(tr_name) != attached_devices.end()) {
				return attached_devices[tr_name].get();
			} 
		} else {
			// std::cerr << "yes" << std::endl;
			std::string name = tr_obj_path.substr(object_path.size()+1);
			// std::cerr << "get_timing_receiver " << name << std::endl;
			return attached_devices[name].get();
		}
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such device");
	}

}