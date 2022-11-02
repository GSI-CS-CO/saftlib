#ifndef EB_PLUGIN_TEMP_SENSOR_HPP_
#define EB_PLUGIN_TEMP_SENSOR_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>


#include <map>
#include <string>

namespace eb_plugin {

class TempSensor {
	etherbone::Device &device;
	eb_address_t ats; // "Altera Temperature Sensor" address
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
