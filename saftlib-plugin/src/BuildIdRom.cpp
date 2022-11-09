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

#include "BuildIdRom.hpp"

#include <saftbus/error.hpp>

#include <sstream>

namespace saftlib {

#define BUILD_ID_ROM_VENDOR_ID 0x00000651
#define BUILD_ID_ROM_DEVICE_ID 0x2d39fa8b

BuildIdRom::BuildIdRom(etherbone::Device &dev) 
	: device(dev)
{
	std::vector<sdb_device> infos_dev;
	device.sdb_find_by_identity(BUILD_ID_ROM_VENDOR_ID, BUILD_ID_ROM_DEVICE_ID, infos_dev);

	if (infos_dev.size() != 1) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "No Build ID found");
	}

	info = (eb_address_t)infos_dev[0].sdb_component.addr_first;
	setupGatewareInfo(info);
}



void BuildIdRom::setupGatewareInfo(uint32_t address)
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


std::map< std::string, std::string > BuildIdRom::getGatewareInfo() const
{
	return gateware_info;
}


std::string BuildIdRom::getGatewareVersion() const
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



}
