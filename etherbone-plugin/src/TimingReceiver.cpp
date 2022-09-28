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

#include "TimingReceiver.hpp"
#include "SAFTd.hpp"
#include "SoftwareActionSink.hpp"
#include "SoftwareActionSink_Service.hpp"

#include <saftbus/error.hpp>

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>



#include "eca_regs.h"
#include "eca_flags.h"
#include "eca_queue_regs.h"
#include "fg_regs.h"
#include "ats_regs.h"


#define BUILD_ID_ROM_VENDOR_ID 0x00000651
#define BUILD_ID_ROM_DEVICE_ID 0x2d39fa8b

namespace eb_plugin {

TimingReceiver::TimingReceiver(SAFTd &saftd, const std::string &n, const std::string &eb_path, saftbus::Container *container)
	: OpenDevice(saftd.get_etherbone_socket(), eb_path)
	, WhiteRabbit(OpenDevice::device)
	, Watchdog(OpenDevice::device)
	, ECA(saftd, OpenDevice::device, object_path, container)
	, object_path(saftd.get_object_path() + "/" + n)
	, name(n)
{
	std::cerr << "TimingReceiver::TimingReceiver" << std::endl;

	if (find_if(name.begin(), name.end(), [](char c){ return !(isalnum(c) || c == '_');} ) != name.end()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
	}

	// watchdog = std::move(std::unique_ptr<WatchdogDriver>(new WatchdogDriver(device)));
	// pps      = std::move(std::unique_ptr<PpsDriver>     (new PpsDriver     (OpenDevice::device, SigLocked)));

	std::vector<sdb_device> infos_dev, ats_dev;
	eb_address_t ats_addr = 0; // not every Altera FPGA model has a temperature sensor, i.e, Altera II

	OpenDevice::device.sdb_find_by_identity(BUILD_ID_ROM_VENDOR_ID, BUILD_ID_ROM_DEVICE_ID, infos_dev);
	OpenDevice::device.sdb_find_by_identity(ATS_SDB_VENDOR_ID,  ATS_SDB_DEVICE_ID, ats_dev);

	// only support super basic hardware for now
	if (infos_dev.size() != 1) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "No Build ID found");
	}

    info = (eb_address_t)infos_dev[0].sdb_component.addr_first;
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
}


void TimingReceiver::setupGatewareInfo(uint32_t address)
{
	eb_data_t buffer[256];

	etherbone::Cycle cycle;
	cycle.open(OpenDevice::device);
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
	WhiteRabbit::getLocked();
	Watchdog::update(); 
	return true;
}


const std::string &TimingReceiver::get_object_path() const
{
	return object_path;
}

void TimingReceiver::Remove() {
	throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver::Remove is deprecated, use SAFTd::Remove instead");
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

eb_plugin::Time TimingReceiver::CurrentTime()
{
	if (!WhiteRabbit::locked) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver is not Locked");
	}
	return eb_plugin::makeTimeTAI(ReadRawCurrentTime());
}

void TimingReceiver::InjectEvent(uint64_t event, uint64_t param, eb_plugin::Time time)
{
	ECA::InjectEventRaw(event, param, time.getTAI());
}


} // namespace saftlib


