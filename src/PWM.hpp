#ifndef saftlib_PWM_SDB_HPP_
#define saftlib_PWM_SDB_HPP_

#include <cstdint>

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#endif

#define PWM_PRODUCT_ID        0x434E5453
#define PWM_VENDOR_ID         0x0000000000000651

#define PWM_TEST_PERIOD    	0x9C3F
#define PWM_TEST_PRESCALER  0x03E7
#define PWM_TEST_PP			0x9CEF03E7
#define PWM_TEST_DUTYCYCLE  0x00004E1F

#define PWM_ADDR_DUTYCYCLE  0x0008

#include <etherbone.h>

#include "SdbDevice.hpp"

namespace saftlib {

class PWM : public SdbDevice {

public:
	PWM(etherbone::Device &device);
	eb_address_t get_start_adr();

	
	void PWM_SetPeriodCounter(uint16_t period);
	void PWM_SetPrescaler(uint16_t prescaler);
	void PWM_SetDutyCycle(uint16_t dutycycle);
	void PWM_WriteConfig(etherbone::address_t pwm_cofig_addr, etherbone::data_t _data, etherbone::address_t pwm_config_addr);
	void PWM_Set_PP(void);
	void PWM_Set_DC(void);
	// @saftbus-export
	void PWM_Test();

private:

	uint16_t _last_period;
	uint16_t _last_prescaler;
	uint16_t _last_pp;
	uint16_t _last_dutycycle;

	//const etherbone::address_t _pwm_addr_0;
	//const etherbone::address_t _pwm_addr_1;

	etherbone::data_t _data;
	etherbone::Cycle _cycle;
	etherbone::exception_t _e;
};

}

#endif