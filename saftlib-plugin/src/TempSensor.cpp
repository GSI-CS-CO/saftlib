#include "TempSensor.hpp"

#include "ats_regs.h"

namespace eb_plugin {

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
