#include "PWM.hpp"

namespace saftlib {

	PWM::PWM(etherbone::Device &device)
		: SdbDevice(device, PWM_VENDOR_ID, PWM_PRODUCT_ID)
	{	
		//_pwm_addr_0 		= 0;
		//_pwm_addr_1 		= 0;
		//_pwm_addr_0 		= adr_first;
		//_pwm_addr_1 		= adr_first + PWM_ADDR_DUTYCYCLE;
		
		/*default values should be minimum values*/
		_last_period 	= 1;
		_last_prescaler = 1;
		_last_pp        = 1;
		_last_dutycycle = 1;
	}

	void PWM::PWM_SetPeriodCounter(uint16_t period)
	{
		if (period != _last_period)
		{
			_last_period = period;

		}
	};


	void PWM::PWM_SetPrescaler(uint16_t prescaler)
	{
		if (prescaler != _last_prescaler)
		{
			_last_prescaler = prescaler;
			
		}
	};


	void PWM::PWM_SetDutyCycle(uint16_t dutycycle)
	{
		if (dutycycle != _last_dutycycle)
		{
			_last_dutycycle = dutycycle;
			
		}
	};

	void PWM::PWM_WriteConfig(etherbone::address_t pwm_cofig_addr, etherbone::data_t _data, etherbone::address_t pwm_config_addr)
	{
		_cycle.open(device);
		_cycle.write(pwm_config_addr, EB_DATA32|EB_BIG_ENDIAN, _data);
		_cycle.close();

	};


	/* test functions */
	void PWM::PWM_Set_PP()
	{
		_data = PWM_TEST_PP;
		_cycle.open(device);
		_cycle.write(adr_first, EB_DATA32|EB_BIG_ENDIAN, _data);
		_cycle.close();
	};

	void PWM::PWM_Set_DC()
	{
		_data = PWM_TEST_DUTYCYCLE;
		_cycle.open(device);
		_cycle.write(adr_first + PWM_ADDR_DUTYCYCLE, EB_DATA32|EB_BIG_ENDIAN, _data);
		_cycle.close();
	};

	void PWM::PWM_Test()
	{
		PWM_Set_PP();
		PWM_Set_DC();
	};



}