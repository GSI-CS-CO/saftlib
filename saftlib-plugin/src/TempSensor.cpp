/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#include "TempSensor.hpp"

#include "ats_regs.h"

namespace saftlib {

TempSensor::TempSensor(etherbone::Device &dev) 
	: device(dev)
	, temperature(0)
{
	std::vector<sdb_device> ats_dev;
	device.sdb_find_by_identity(ATS_SDB_VENDOR_ID,  ATS_SDB_DEVICE_ID, ats_dev);
	if (ats_dev.size() > 0) {
		ats = (eb_address_t)ats_dev[0].sdb_component.addr_first;
	}
}


bool TempSensor::getTemperatureSensorAvail() const
{
	return ats != 0;
}

int32_t TempSensor::CurrentTemperature()
{
	if (ats) {
		eb_data_t data;
		device.read(ats + ALTERA_TEMP_DEGREE, EB_DATA32, &data);

		if (data != 0xDEADC0DE) {
			temperature = (int32_t) data;
		}
	}

	return temperature;
}


} // namespace
