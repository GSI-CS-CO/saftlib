/*  Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#include "SdbDevice.hpp"

#include <saftbus/error.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

namespace saftlib {

	SdbDevice::SdbDevice(etherbone::Device &dev, uint32_t VENDOR_ID, uint32_t DEVICE_ID, bool throw_if_not_found) 
		: device(dev)
	{
		std::vector<sdb_device> devs;
		device.sdb_find_by_identity(VENDOR_ID, DEVICE_ID, devs);

		if (devs.size() < 1) {
			std::ostringstream msg;
			msg << "no SDB device with VENDOR_ID=0x" << std::hex << std::setw(8) << std::setfill('0') << VENDOR_ID 
				<<               " and DEVICE_ID=0x" << std::hex << std::setw(8) << std::setfill('0') << DEVICE_ID;

			if (throw_if_not_found) {
				throw saftbus::Error(saftbus::Error::FAILED, msg.str());
			}
			adr_first = 0;
			return;
		}
		if (devs.size() > 1) {
			std::cerr << "more than one SDB device found on hardware, taking the first one" << std::endl;
		}
		adr_first = static_cast<eb_address_t>(devs[0].sdb_component.addr_first);
	}

	SdbDevice::~SdbDevice() {
	}


}

