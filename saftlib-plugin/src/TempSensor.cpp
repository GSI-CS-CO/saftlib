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

TempSensor::TempSensor(etherbone::Device &device) 
	: SdbDevice(device, ATS_SDB_VENDOR_ID,  ATS_SDB_DEVICE_ID, false)
	, temperature(0)
{
}


bool TempSensor::getTemperatureSensorAvail() const
{
	return adr_first != 0;
}

int32_t TempSensor::CurrentTemperature()
{
	if (adr_first) {
		eb_data_t data;
		device.read(adr_first + ALTERA_TEMP_DEGREE, EB_DATA32, &data);

		if (data != 0xDEADC0DE) {
			temperature = (int32_t) data;
		}
	}

	return temperature;
}


} // namespace
