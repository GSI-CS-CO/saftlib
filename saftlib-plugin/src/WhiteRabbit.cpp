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

#include "WhiteRabbit.hpp"

#include <saftbus/error.hpp>

#include <iostream>

namespace saftlib {

#define WR_API_VERSION          4

#if WR_API_VERSION <= 3
#define WR_PPS_GEN_ESCR_MASK    0x6       //bit 1: PPS valid, bit 2: TS valid
#else
#define WR_PPS_GEN_ESCR_MASK    0xc       //bit 2: PPS valid, bit 3: TS valid
#endif
#define WR_PPS_GEN_ESCR         0x1c      //External Sync Control Register

#define WR_PPS_VENDOR_ID        0xce42
#define WR_PPS_DEVICE_ID        0xde0d8ced


WhiteRabbit::WhiteRabbit(etherbone::Device &dev)
	: device(dev)
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

bool WhiteRabbit::getLocked() const
{
	eb_data_t data;
	device.read(pps + WR_PPS_GEN_ESCR, EB_DATA32, &data);
	bool newLocked = (data & WR_PPS_GEN_ESCR_MASK) == WR_PPS_GEN_ESCR_MASK;

	/* Update signal */
	if (newLocked != locked) {
		locked = newLocked;
		Locked(locked);
	}

	return newLocked;
}




}

