/*  Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#ifndef saftlib_TEMP_SENSOR_HPP_
#define saftlib_TEMP_SENSOR_HPP_

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

class TempSensor : public SdbDevice {
    mutable int32_t temperature;
public:
	TempSensor(etherbone::Device &device);

	/// @brief Check if a temperature sensor is available
	/// @return Check if a temperature sensor is available
	///
	/// in a timing receiver.
	///
	// @saftbus-export
	bool getTemperatureSensorAvail() const;

	/// @brief The current temperature in degree Celsius.
	/// @return         Temperature in degree Celsius.
	///
	/// The valid temperature range is from -70 to 127 degree Celsius.
	/// The data type is 32-bit signed integer.
	///
	// @saftbus-export
	int32_t CurrentTemperature();


};


}


#endif
