#include "PWM.hpp"

#include <iostream>

namespace saftlib {

	PWM::PWM(etherbone::Device &device)
		: SdbDevice(device, PWM_VENDOR_ID, PWM_PRODUCT_ID)
	{	
		_pwm_addr_0 		= adr_first;
		
		/*default values should be minimum values*/
		_last_period 	= 1;
		_last_prescaler = 1;
		_last_pp        = 1;
		_last_dutycycle = 1;
	}

	void PWM::PWM_Calc_ActualFreq(int channel, int req_freq)
	{
		/* since the PWM period needs to be a multiple of RefClk Time */
		/* and gencore module has min PWM period of 240 */

		double actual_freq, req_period_ns, remainder = 0.0;
		int actual_period_ns  = 0;

		req_period_ns 	= (1.0 / req_freq) * PWM_S_TO_NS;

		/* check if we need to round up or down */
		remainder 	= fmod(req_period_ns, PWM_REF_CLK_TIME_NS);
		req_period_ns -= remainder;
		if (remainder > (double)  PWM_REF_CLK_TIME_NS / 2.0){req_period_ns += (double)  PWM_REF_CLK_TIME_NS;}

		actual_period_ns = (int) req_period_ns;
		actual_freq = 1.0 / ((double) actual_period_ns / PWM_S_TO_NS );

		std::cout << "Actual period will be " << actual_period_ns  << std::endl;
		std::cout << "Actual frequency will be " << actual_freq  << std::endl;

		pwm_channels[channel].used 			= true;
		pwm_channels[channel].pwm_period 	= (int) actual_period_ns;
		pwm_channels[channel].pwm_freq 		= (int) actual_freq;

		std::cout << "Actual period will be " << pwm_channels[channel].pwm_period  << std::endl;
		std::cout << "Actual frequency will be " << pwm_channels[channel].pwm_freq  << std::endl;

	};

	void PWM::PWM_Calc_ActualDutyCycle(int channel, int req_dc)
	{
		double req_dc_period_ns, remainder = 0.0;

		req_dc_period_ns = (pwm_channels[channel].pwm_period / 100.0) * (req_dc);

		/* check if we need to round up or down */
		remainder 	= fmod(req_dc_period_ns, PWM_REF_CLK_TIME_NS);
		req_dc_period_ns -= remainder;
		if (remainder > (double)  PWM_REF_CLK_TIME_NS / 2.0){req_dc_period_ns += (double)  PWM_REF_CLK_TIME_NS;}

		pwm_channels[channel].pwm_actual_duty_cycle = req_dc_period_ns / (pwm_channels[channel].pwm_period / 100);
		pwm_channels[channel].pwm_duty_cycle_value	= (int) req_dc_period_ns / PWM_REF_CLK_TIME_NS / (pwm_channels[channel].pwm_prescaler_value + 1);

		std::cout << "Actual duty cycle will be " << pwm_channels[channel].pwm_actual_duty_cycle  << std::endl;
		std::cout << "Actual duty cycle will be " << pwm_channels[channel].pwm_actual_duty_cycle  << std::endl;

	}; 

	void PWM::PWM_Calc_ModuleInputs(int channel)
	{
		int multiple = 0;
		/* TODO move into try with custom exception */
		if(pwm_channels[channel].used){

			multiple = pwm_channels[channel].pwm_period / PWM_REF_CLK_TIME_NS;
			pwm_channels[channel].pwm_prescaler_value 	= PWM_DEFAULT_PRESCALER;
			pwm_channels[channel].pwm_period_value 			= multiple / (pwm_channels[channel].pwm_prescaler_value + 1);
			std::cout << "Prescaler_value was calculated as " << pwm_channels[channel].pwm_prescaler_value << std::endl;
			std::cout << "Period_value was calculated as " << pwm_channels[channel].pwm_period_value << std::endl;
			std::cout << "Duty cycle value was calculated as " << pwm_channels[channel].pwm_duty_cycle_value  << std::endl;

		} else{

			std::cerr << "Channel not initiated - please fist initialize channel " << channel << std::endl;
		}

	};

	/* test functions */
	void PWM::PWM_Set_Test_PP()
	{
		_cycle.open(device);
		_cycle.write(_pwm_addr_0, EB_DATA32|EB_BIG_ENDIAN, PWM_TEST_PP);
		_cycle.close();
	};

	void PWM::PWM_Set_Test_DC(etherbone::address_t pwm_target_reg, etherbone::data_t data_to_send)
	{
		_cycle.open(device);
		_cycle.write(pwm_target_reg, EB_DATA32|EB_BIG_ENDIAN, data_to_send);
		_cycle.close();
	};

	void PWM::PWM_Pack_Data(int channel)
	{
		union wb_data {
  				std::uint16_t prescaler_value;      
    			std::uint16_t period_value;         
    			std::uint32_t data_to_be_written;  
		};

		wb_data data_to_send;

		data_to_send.prescaler_value = (uint16_t) pwm_channels[channel].pwm_prescaler_value;
		data_to_send.period_value 	= (uint16_t) pwm_channels[channel].pwm_period_value;

		_PP_data = data_to_send.data_to_be_written;
		_DT_data = (etherbone::data_t) pwm_channels[channel].pwm_duty_cycle_value;

		std::cout << "Sending _PP_data as " << std::hex << _PP_data  << std::endl;
		std::cout << "Sending _DT_data as " << std::hex << _DT_data << std::endl;
	};

	void PWM::PWM_Send_Config(int channel)
	{	
		etherbone::address_t _channel_config;
		_channel_config = PWM_Get_ChannelRegAdr(channel);
		try
		{
		    _cycle.open(device);
			_cycle.write(_pwm_addr_0, EB_DATA32|EB_BIG_ENDIAN, _PP_data);
			_cycle.write(_channel_config, EB_DATA32|EB_BIG_ENDIAN, _DT_data);
			_cycle.close();
		 }
		catch (etherbone::exception_t error) 
		{
		    /* Catch error(s) */
		    std::cerr << "Failed to invoke method: " << error << std::endl;
		}
	};

	void PWM::PWM_Set_Channel(int channel, int req_freq, int req_duty_cycle)
	{	
		pwm_channels[channel].used = true;
		std::cout << "PWM_Set_Channel called ...... " << pwm_channels[channel].pwm_duty_cycle_value  << std::endl;
		PWM_Calc_ActualFreq(channel, req_freq);
		PWM_Calc_ModuleInputs(channel);
		PWM_Calc_ActualDutyCycle(channel, req_duty_cycle);
		PWM_Calc_ModuleInputs(channel);
		PWM_Pack_Data(channel);
		PWM_Send_Config(channel);
	};

	etherbone::address_t PWM::PWM_Get_ChannelRegAdr(int channel){

		int offset = 0;
		etherbone::address_t pwm_target_reg = _pwm_addr_0;

		/* TODO: move into enum or array*/
		switch(channel){
			case 1: { offset = PWM_DT_CH_0_OFFSET; break; }
			case 2: { offset = PWM_DT_CH_1_OFFSET; break; }
			case 3: { offset = PWM_DT_CH_2_OFFSET; break; }
			case 4: { offset = PWM_DT_CH_3_OFFSET; break; }
			case 5: { offset = PWM_DT_CH_4_OFFSET; break; }
			case 6: { offset = PWM_DT_CH_5_OFFSET; break; }
			case 7: { offset = PWM_DT_CH_6_OFFSET; break; }
			case 8: { offset = PWM_DT_CH_7_OFFSET; break; }
			default:
				break;
		}
		pwm_target_reg += offset;

		return pwm_target_reg;
	};

	void PWM::PWM_Test(int channel)
	{	
		etherbone::data_t data_to_send = PWM_TEST_DUTYCYCLE;
		etherbone::address_t pwm_target_reg = PWM_Get_ChannelRegAdr(channel);
 
		PWM_Set_Test_PP();
		PWM_Set_Test_DC(pwm_target_reg, data_to_send);
	};



}