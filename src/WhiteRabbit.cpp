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

#include "WhiteRabbit.hpp"
#include "Owned.hpp"

#include <saftbus/error.hpp>

#include <unistd.h>

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

// GSI wr_info device (modules/wr_info) as replacement for the CERN WR PPS generator
#define WR_INFO_VENDOR_ID       0x00000651
#define WR_INFO_DEVICE_ID       0x77722d69
#define WR_INFO_LOCKED_BIT      0x1

WhiteRabbit::WhiteRabbit(etherbone::Device &device)
	: SdbDevice(device, WR_PPS_VENDOR_ID, WR_PPS_DEVICE_ID, false)
	, _has_wr_info_unit(false)
{
	if (!found) {
		// no CERN WR PPS generator found, use the GSI wr_info device instead
		SdbDevice wr_info_device(device, WR_INFO_VENDOR_ID, WR_INFO_DEVICE_ID);
		adr_first = wr_info_device.get_adr_base();
		_has_wr_info_unit = true;
	}
	getLocked();
}

bool WhiteRabbit::getLocked() const
{
	eb_data_t data;
	bool newLocked;
	if (_has_wr_info_unit) {
		// GSI wr_info: lock info is the first bit at the first address (no offset)
		device.read(adr_first, EB_DATA32, &data);
		newLocked = (data & WR_INFO_LOCKED_BIT) != 0;
	} else {
		device.read(adr_first + WR_PPS_GEN_ESCR, EB_DATA32, &data);
		newLocked = (data & WR_PPS_GEN_ESCR_MASK) == WR_PPS_GEN_ESCR_MASK;
	}

	/* Update signal */
	if (newLocked != locked) {
		locked = newLocked;
		Locked(locked);
	}

	return newLocked;
}




}

