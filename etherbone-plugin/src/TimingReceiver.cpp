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
#include <algorithm>

#include <saftbus/error.hpp>

#include "SAFTd.hpp"
#include "TimingReceiver.hpp"
#include "SoftwareActionSink.hpp"
#include "SoftwareActionSink_Service.hpp"


#include "eca_regs.h"
#include "eca_flags.h"
#include "eca_queue_regs.h"
#include "fg_regs.h"
#include "ats_regs.h"

#include "EcaDriver.hpp"

namespace eb_plugin {

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

class WatchdogDriver {
	etherbone::Device &device;
	eb_address_t watchdog;
	eb_data_t watchdog_value;
public:
	WatchdogDriver(etherbone::Device &device);
	bool aquire();
	void update();
};

WatchdogDriver::WatchdogDriver(etherbone::Device &dev) 
	: device(dev) 
{
	std::vector<sdb_device> watchdogs_dev;
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0xb6232cd3, watchdogs_dev);
	if (watchdogs_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no watchdog device found on hardware");
	}
	if (watchdogs_dev.size() > 1) {
		std::cerr << "more than one watchdog device found on hardware, taking the first one" << std::endl;
	}
	watchdog = static_cast<eb_address_t>(watchdogs_dev[0].sdb_component.addr_first);
	if (!aquire()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Watchdog on Timing Receiver already locked");
	}
}
bool WatchdogDriver::aquire() {
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
void WatchdogDriver::update() {
	device.write(watchdog, EB_DATA32, watchdog_value);
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

#define WR_API_VERSION          4

#if WR_API_VERSION <= 3
#define WR_PPS_GEN_ESCR_MASK    0x6       //bit 1: PPS valid, bit 2: TS valid
#else
#define WR_PPS_GEN_ESCR_MASK    0xc       //bit 2: PPS valid, bit 3: TS valid
#endif
#define WR_PPS_GEN_ESCR         0x1c      //External Sync Control Register

#define WR_PPS_VENDOR_ID        0xce42
#define WR_PPS_DEVICE_ID        0xde0d8ced

class PpsDriver {
	friend class TimingReceiver;
	etherbone::Device &device;
	std::function<void(bool locked)> &SigLocked;
	eb_address_t pps;
	mutable bool locked;
public:
	PpsDriver(etherbone::Device &dev, std::function<void(bool locked)> &sl);
	bool getLocked() const;
};

PpsDriver::PpsDriver(etherbone::Device &dev, std::function<void(bool locked)> &sl)
	: device(dev)
	, SigLocked(sl)
{
	std::vector<sdb_device> pps_dev;
	device.sdb_find_by_identity(WR_PPS_VENDOR_ID, WR_PPS_DEVICE_ID, pps_dev);
	if (pps_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no WR-PPS device found on hardware");
	}
	if (pps_dev.size() > 1) {
		std::cerr << "more than one WR-PPS device found on hardware, taking the first one" << std::endl;
	}
	pps = static_cast<eb_address_t>(pps_dev[0].sdb_component.addr_first);
	getLocked();
}

bool PpsDriver::getLocked() const
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

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

#define BUILD_ID_ROM_VENDOR_ID 0x00000651
#define BUILD_ID_ROM_DEVICE_ID 0x2d39fa8b

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
	pps->getLocked();
	watchdog->update();
	return true;
}

TimingReceiver::TimingReceiver(SAFTd *saftd, const std::string &n, const std::string eb_path, saftbus::Container *container)
	: object_path(saftd->get_object_path() + "/" + n)
	, name(n)
	, etherbone_path(eb_path)
{
	std::cerr << "TimingReceiver::TimingReceiver" << std::endl;

	if (find_if(name.begin(), name.end(), [](char c){ return !(isalnum(c) || c == '_');} ) != name.end()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
	}

	stat(etherbone_path.c_str(), &dev_stat);
	device.open(saftd->get_etherbone_socket(), etherbone_path.c_str());

	try {
		watchdog = std::move(std::unique_ptr<WatchdogDriver>(new WatchdogDriver(device)));
		pps      = std::move(std::unique_ptr<PpsDriver>     (new PpsDriver     (device, SigLocked)));
		eca      = std::move(std::unique_ptr<EcaDriver>     (new EcaDriver     (saftd, device, object_path, container)));
	} catch (etherbone::exception_t &e) {
		device.close();
		chmod(etherbone_path.c_str(), dev_stat.st_mode);
		throw;		
	}

	// std::vector<etherbone::sdb_msi_device> mbx_msi_dev;
	std::vector<sdb_device> infos_dev, ats_dev;
	eb_address_t ats_addr = 0; // not every Altera FPGA model has a temperature sensor, i.e, Altera II

	device.sdb_find_by_identity(BUILD_ID_ROM_VENDOR_ID, BUILD_ID_ROM_DEVICE_ID, infos_dev);
	device.sdb_find_by_identity(ATS_SDB_VENDOR_ID,  ATS_SDB_DEVICE_ID, ats_dev);

	// only support super basic hardware for now
	if (infos_dev.size() != 1) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has insuficient hardware resources");
	}

    info      = (eb_address_t)infos_dev[0].sdb_component.addr_first;
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
	std::cerr << "TimingReceiver::~TimingReceiver" << std::endl;
	std::cerr << "saftbus::Loop::get_default().remove(poll_timeout_source)" << std::endl;
	saftbus::Loop::get_default().remove(poll_timeout_source);
	std::cerr << "device.close()" << std::endl;
	device.close();
	std::cerr << "fix device file" << std::endl;
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


std::string TimingReceiver::getGatewareVersion() const
{
	std::map< std::string, std::string >         gatewareInfo;
	std::map<std::string, std::string>::iterator j;
	std::string                                  rawVersion;
	std::string                                  findString = "-v";
	int                                          pos = 0;

	gatewareInfo = getGatewareInfo();
	j = gatewareInfo.begin();        // build date
	j++;                             // gateware version
	rawVersion = j->second;
	pos = rawVersion.find(findString, 0);

	if ((pos <= 0) || (((pos + findString.length()) >= rawVersion.length()))) return ("N/A");

	pos = pos + findString.length(); // get rid of findString '-v'

	return(rawVersion.substr(pos, rawVersion.length() - pos));
}

bool TimingReceiver::getLocked() const
{
	return pps->getLocked();
}



void TimingReceiver::InjectEvent(uint64_t event, uint64_t param, uint64_t time)
{
	// InjectEvent(event,param,eb_plugin::makeTimeTAI(time));
	eca->InjectEvent(event,param,eb_plugin::makeTimeTAI(time));
}

void TimingReceiver::InjectEvent(uint64_t event, uint64_t param, eb_plugin::Time time)
{
	eca->InjectEvent(event, param, time);
}


std::string TimingReceiver::NewSoftwareActionSink(const std::string& name) {
	return eca->NewSoftwareActionSink(name);
}

std::map< std::string, std::string > TimingReceiver::getSoftwareActionSinks() const {
	return eca->getSoftwareActionSinks();
}

SoftwareActionSink *TimingReceiver::getSoftwareActionSink(const std::string & object_path) {
	return eca->getSoftwareActionSink(object_path);
}

eb_plugin::Time TimingReceiver::CurrentTime()
{
	if (!pps->locked) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver is not Locked");
	}
	return eb_plugin::makeTimeTAI(eca->ReadRawCurrentTime());
}



} // namespace saftlib


