/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS


#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>

#include <saftbus/error.hpp>

#include "TimingReceiver.hpp"

#include "eca_regs.h"
#include "eca_queue_regs.h"
#include "fg_regs.h"
#include "ats_regs.h"


namespace eb_plugin {

// Device::irqMap Device::irqs;   // this is in globals.cpp
// Device::msiQueue Device::msis; // this is in globals.cpp

bool TimingReceiver::aquire_watchdog() {
	// try to acquire watchdog
	eb_data_t retry;
	device.read(watchdog, EB_DATA32, &watchdog_value);
	if ((watchdog_value & 0xFFFF) != 0) {
		return false;
	}
	device.write(watchdog, EB_DATA32, watchdog_value);
	device.read(watchdog, EB_DATA32, &retry);
	if (((retry ^ watchdog_value) >> 16) != 0) {
		return false;
	}
	return true;
}

void TimingReceiver::setupGatewareInfo(uint32_t address)
{
	eb_data_t buffer[256];

	etherbone::Cycle cycle;
	cycle.open(device);
	for (unsigned i = 0; i < sizeof(buffer)/sizeof(buffer[0]); ++i) {
		cycle.read(address + i*4, EB_DATA32, &buffer[i]);
	}
	cycle.close();

	std::string str;
	for (unsigned i = 0; i < sizeof(buffer)/sizeof(buffer[0]); ++i) {
		str.push_back((buffer[i] >> 24) & 0xff);
		str.push_back((buffer[i] >> 16) & 0xff);
		str.push_back((buffer[i] >>  8) & 0xff);
		str.push_back((buffer[i] >>  0) & 0xff);
	}

	std::stringstream ss(str);
	std::string line;
	while (std::getline(ss, line))
	{
		if (line.empty() || line[0] == ' ') continue; // skip empty/history lines

		std::string::size_type offset = line.find(':');
		if (offset == std::string::npos) continue; // not a field
		if (offset+2 >= line.size()) continue;
		std::string value(line, offset+2);

		std::string::size_type tail = line.find_last_not_of(' ', offset-1);
		if (tail == std::string::npos) continue;
		std::string key(line, 0, tail+1);

		// store the field
		gateware_info[key] = value;
	}
}

std::map< std::string, std::string > TimingReceiver::getGatewareInfo() const
{
  return gateware_info;
}

bool TimingReceiver::poll()
{
	std::cerr << "TimingReceiver::poll()" << std::endl;
	getLocked();
	device.write(watchdog, EB_DATA32, watchdog_value);
	return true;
}


TimingReceiver::TimingReceiver(saftbus::Container *cont, SAFTd *sd, etherbone::Socket &socket, const std::string &obj_path, const std::string &n, const std::string eb_path)
	: container(cont)
	, saftd(sd)
	, object_path(obj_path)
	, name(n)
	, etherbone_path(eb_path)
{
	std::cerr << "TimingReceiver::TimingReceiver" << std::endl;
	stat(etherbone_path.c_str(), &dev_stat);
	object_path.append("/");
	object_path.append(name);
	device.open(socket, etherbone_path.c_str());

	// // This just reads the MSI address range out of the ehterbone config space registers
	// // It does not actually enable anything ... MSIs also work without this
	device.enable_msi(&first, &last);

	// Confirm the device is an aligned power of 2
	eb_address_t size = last - first;
	if (((size + 1) & size) != 0) {
		device.close();
		chmod(etherbone_path.c_str(), dev_stat.st_mode);
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has strange sized MSI range");
	}
	if ((first & size) != 0) {
		device.close();
		chmod(etherbone_path.c_str(), dev_stat.st_mode);
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has unaligned MSI first address");
	}


	std::vector<etherbone::sdb_msi_device> ecas_dev, mbx_msi_dev;
	std::vector<sdb_device> streams_dev, infos_dev, watchdogs_dev, scubus_dev, pps_dev, mbx_dev, ats_dev;
	eb_address_t ats_addr = 0; // not every Altera FPGA model has a temperature sensor, i.e, Altera II

	device.sdb_find_by_identity_msi(ECA_SDB_VENDOR_ID, ECA_SDB_DEVICE_ID, ecas_dev);
	device.sdb_find_by_identity_msi(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx_msi_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x8752bf45, streams_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x2d39fa8b, infos_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0xb6232cd3, watchdogs_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x9602eb6f, scubus_dev);
	device.sdb_find_by_identity(0xce42, 0xde0d8ced, pps_dev);
	device.sdb_find_by_identity(ATS_SDB_VENDOR_ID,  ATS_SDB_DEVICE_ID, ats_dev);
	device.sdb_find_by_identity(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx_dev);

	// only support super basic hardware for now
	if (ecas_dev.size() != 1 || streams_dev.size() != 1 || infos_dev.size() != 1 || watchdogs_dev.size() != 1 
		|| pps_dev.size() != 1 || mbx_dev.size() != 1 || mbx_msi_dev.size() != 1) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has insuficient hardware resources");
	}

    stream    = (eb_address_t)streams_dev[0].sdb_component.addr_first;
    info      = (eb_address_t)infos_dev[0].sdb_component.addr_first;
    watchdog  = (eb_address_t)watchdogs_dev[0].sdb_component.addr_first;
    pps       = (eb_address_t)pps_dev[0].sdb_component.addr_first;

    if (!aquire_watchdog()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Timing Receiver already locked");
    }
	setupGatewareInfo(info);

	// update locked status ...
	poll();
	//    ... and repeat every 1s 
	poll_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&TimingReceiver::poll, this), std::chrono::milliseconds(1000), std::chrono::milliseconds(1000)
		);

}

TimingReceiver::~TimingReceiver() 
{
	saftbus::Loop::get_default().remove(poll_timeout_source);

	std::cerr << "TimingReceiver::~TimingReceiver" << std::endl;
	device.close();
	chmod(etherbone_path.c_str(), dev_stat.st_mode);
}

const std::string &TimingReceiver::get_object_path() const
{
	return object_path;
}

void TimingReceiver::Remove() {
	throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver::Remove is deprecated, use SAFTd::Remove instead");
}
std::string TimingReceiver::getEtherbonePath() const
{
	return etherbone_path;
}
std::string TimingReceiver::getName() const
{
	return name;
}

#define WR_API_VERSION          4

#if WR_API_VERSION <= 3
#define WR_PPS_GEN_ESCR_MASK    0x6       //bit 1: PPS valid, bit 2: TS valid
#else
#define WR_PPS_GEN_ESCR_MASK    0xc       //bit 2: PPS valid, bit 3: TS valid
#endif
#define WR_PPS_GEN_ESCR         0x1c      //External Sync Control Register

bool TimingReceiver::getLocked() const
{
  eb_data_t data;
  device.read(pps + WR_PPS_GEN_ESCR, EB_DATA32, &data);
  bool newLocked = (data & WR_PPS_GEN_ESCR_MASK) == WR_PPS_GEN_ESCR_MASK;
  
  /* Update signal */
  if (newLocked != locked) {
    locked = newLocked;
    if (SigLocked) {
	    SigLocked(locked);
    }
  }
  
  return newLocked;
}

} // namespace saftlib
