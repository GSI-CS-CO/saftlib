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

#ifndef saftlib_SDB_DEVICE_HPP_
#define saftlib_SDB_DEVICE_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

namespace saftlib {

/// @brief SdbDevices calls sdb_find_by_identity and keeps the starting address of the device registers.
/// 
/// This class is supposed to be a base class of all driver classes that interact with a single SDB device 
/// on the hardware. Deriving from this class avoids to rewrite the boilerplate code to identify a single 
/// device. However, it does not support the case when multiple devices with same VENDOR_ID and DEVICE_ID 
/// exist. This class always uses the first device listed if more then one exists. It throws an exception 
/// if the device is not found at all.
///
/// @param dev the etherbone::Device 
/// @param VENDOR_ID vendor id of the device
/// @param DEVICE_ID device id of the device
/// @param throw_if_not_found in some cases an exception is not desired in which case this parameter can be set to false.
class SdbDevice {
	friend class SAFTd; // SAFTd can use an MsiDevice to register a callbak on MSIs
protected:
	eb_address_t adr_first;
	etherbone::Device &device;
public:
	SdbDevice(etherbone::Device &device, uint32_t VENDOR_ID, uint32_t DEVICE_ID, bool throw_if_not_found = true);
	virtual ~SdbDevice();
};

}

#endif
