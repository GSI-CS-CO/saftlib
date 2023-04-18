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

#ifndef saftlib_BUILD_ID_ROM_HPP_
#define saftlib_BUILD_ID_ROM_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include "SdbDevice.hpp"

#include <map>
#include <string>

namespace saftlib {

/// @brief Representation of the SDB device with build id information.
/// It can be used to obtain strings with gateware info and version numbers.
class BuildIdRom : public SdbDevice {
	std::map<std::string, std::string> gateware_info;
	void setupGatewareInfo(uint32_t address);
public:
	BuildIdRom(etherbone::Device &device);

	/// @brief Key-value map of hardware build information
	/// @return Key-value map of hardware build information
	///
	// @saftbus-export
	std::map< std::string, std::string > getGatewareInfo() const;

	/// @brief Hardware build version
	/// @return "major.minor.tiny" if version is valid (or "N/A" if not available)
	///
	// @saftbus-export
	std::string getGatewareVersion() const;

};


}


#endif
