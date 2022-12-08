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

#include "MsiDevice.hpp"

#include <saftbus/error.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

namespace saftlib {

    MsiDevice::MsiDevice(etherbone::Device &device, uint32_t VENDOR_ID, uint32_t DEVICE_ID) 
        : SdbDevice(device, VENDOR_ID, DEVICE_ID)
    {
		std::vector<etherbone::sdb_msi_device> msis;
		device.sdb_find_by_identity_msi(VENDOR_ID, DEVICE_ID, msis);
		if (msis.size() < 1) {
			std::ostringstream msg;
			msg << "no SDB-MSI device with VENDOR_ID=0x" << std::hex << std::setw(8) << std::setfill('0') << VENDOR_ID 
			    <<                   " and DEVICE_ID=0x" << std::hex << std::setw(8) << std::setfill('0') << DEVICE_ID;
			throw saftbus::Error(saftbus::Error::FAILED, msg.str());
		}
		if (msis.size() > 1) {
			std::cerr << "more than one MSI device found on hardware, taking the first one" << std::endl;
		}
		msi_device = msis[0];

    }


}

