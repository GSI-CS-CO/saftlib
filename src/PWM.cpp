/*  Copyright (C) 2011-2016, 2021-2024 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
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

#include "PWM.hpp"

#include <iostream>

namespace saftlib {

	PWM::PWM(etherbone::Device &device)
		: SdbDevice(device, PWM_VENDOR_ID, PWM_PRODUCT_ID)
	{	
		_pwm_addr_0 		= adr_first;

	}

	void PWM::PWM_Calc_ActualFreq(int channel, int req_freq)
	{
		/* the PWM period needs to be a multiple of PWM_REF_CLK_TIME_NS */
		/* and the general cores module has minimum PWM_REF_MIN_PERIOD_NS */

		long actual_period_ns  = 0;
		double req_period_ns, actual_freq = 0.0;

		std::cout << "PWM_Calc_ActualFreq called with " << +req_freq  << std::endl;

		/* make sure the requested frequency is not to big */
		if(req_freq > PWM_REF_MAX_FREQ_HZ) {
			req_freq = PWM_REF_MAX_FREQ_HZ;
			std::cout << "Requested frequency had to be adjusted to maximum possible frequency  " << +req_freq  << std::endl;
		}

		/* calculate period[ns] of requested frequency as a double*/
		req_period_ns = (1.0 / req_freq) * PWM_S_TO_NS;
		std::cout << "Requested period was " << req_period_ns << std::endl;

		/* approximate the above period by finding the closest multiple of the used clock period */
		auto remainder = std::fmod(req_period_ns, PWM_REF_CLK_TIME_NS);
		std::cout << "std::fmod(req_period_ns, PWM_REF_CLK_TIME_NS) returned " << remainder << std::endl;
		actual_period_ns = req_period_ns - remainder;
		if (remainder > (double)  PWM_REF_CLK_TIME_NS / 2.0){actual_period_ns += PWM_REF_CLK_TIME_NS;}

		/* calculate the resulting frequency */	
		actual_freq = 1.0 / (((double) actual_period_ns) / PWM_S_TO_NS );

		std::cout << "Actual period in double will be " << actual_period_ns  << std::endl;
		std::cout << "Actual frequency in double for now will be " << actual_freq  << std::endl;

		pwm_channels[channel].used 					= true;
		pwm_channels[channel].pwm_period_long 		= actual_period_ns;
		pwm_channels[channel].pwm_freq_double		= actual_freq;

	};

	std::vector<long> PWM::PWM_Calc_Factors(long num_to_factorise)
	{	
		/* simple trial factorization, can be improved by divisibility checks */
		std::vector<long> factors;

		for (long divisor = 2; divisor * divisor <= num_to_factorise; divisor++) {
        	while (num_to_factorise % divisor == 0) {
            	factors.push_back(divisor);
            	num_to_factorise /= divisor;
        	}
    	}
    	if (num_to_factorise > 1)
        	factors.push_back(num_to_factorise);

        /* remove */
    	for (auto i : factors)
        	std::cout << i << " ";
        std::cout << std::endl;

    	return factors;
    };

	void PWM::PWM_Calc_PeriodPrescaler(int channel)
	{
		/* the period must be a multiple of PWM_REF_CLK_TIME_NS */
		/* hence the necessary steps(period / PWM_REF_CLK_TIME_NS) need to be factorized */
		/* into integer prescaler and period counter which each need to fit into 16 bits */

		long steps = pwm_channels[channel].pwm_period_long / PWM_REF_CLK_TIME_NS;
		std::cout << "Steps needed are " << +steps << std::endl;

		/* simple trial factorization  */
		std::vector<long> factors = {0};
		factors = PWM_Calc_Factors(steps);

		/* the used module has minimum value of 2 */
		/* since the smallest prime is 2 we can start with 1*/
		uint16_t current_prescaler_value = 1;
		long current_period_counter_value = steps;

		/* try to find fitting factors */
		for(const long& factor : factors){

			/* beware: no PWM_MIN_PRESCALER comparison */
			/* only implicit for PWM_MIN_PRESCALER = 2 */
			current_prescaler_value *= factor;

			std::cout << "current_prescaler_value " << current_prescaler_value << std::endl;
		
			current_period_counter_value = steps / current_prescaler_value;
			std::cout << "current_period_counter_value " << current_period_counter_value << std::endl;

			/* make sure it fits into 16 bit */
			if(current_period_counter_value < PWM_MAX_MOD_VALUE) {break;}
			/* beware: steps which are prime bigger PWM_MAX_MOD_VALUE will not be caught ! */
		}
		std::cout << "after loop - current_period_counter_value " << current_period_counter_value << std::endl;

		pwm_channels[channel].pwm_prescaler_value = static_cast<uint16_t>(current_prescaler_value);
		pwm_channels[channel].pwm_period_value = static_cast<uint16_t>(current_period_counter_value);
		std::cout << "Final prescaler value was set as " << +pwm_channels[channel].pwm_prescaler_value << std::endl;
		std::cout << "                          in hex " << std::hex <<pwm_channels[channel].pwm_prescaler_value << std::endl;
		std::cout << "Final period counter value was set as " << +pwm_channels[channel].pwm_period_value << std::endl;
		std::cout << "                               in hex " << std::hex << pwm_channels[channel].pwm_period_value << std::endl;

	};

	void PWM::PWM_Calc_ActualDutyCycle(int channel, int req_dc)
	{	
		/* the period must be a multiple of PWM_REF_CLK_TIME_NS */
		/* hence the necessary steps(period / PWM_REF_CLK_TIME_NS) need to be factorized */
		/* into integer prescaler and period counter which each need to fit into 16 bits */

		double req_dc_period_ns, remainder = 0.0;

		req_dc_period_ns = (double)(pwm_channels[channel].pwm_period_long / 100.0) * (req_dc);
		std::cout << "req_dc_period_ns " << +req_dc_period_ns  << std::endl;

		double pwm_prescaler_x_clk = (double) ((pwm_channels[channel].pwm_prescaler_value+1) * PWM_REF_CLK_TIME_NS);
		std::cout << "pwm_prescaler_x_clk " << +pwm_prescaler_x_clk  << std::endl;

		/* check if we need to round up or down */
		remainder 	= std::fmod(pwm_prescaler_x_clk, req_dc_period_ns);
		req_dc_period_ns = req_dc_period_ns - remainder;
		std::cout << "remainder " << remainder  << std::endl;
		if (remainder > (double)  pwm_prescaler_x_clk / 2.0){req_dc_period_ns += (double)  pwm_prescaler_x_clk;}
		std::cout << "req_dc_period_ns after fmod " << req_dc_period_ns  << std::endl;

		pwm_channels[channel].pwm_actual_duty_cycle = (double) req_dc_period_ns / (pwm_channels[channel].pwm_period_long / 100.0);
		pwm_channels[channel].pwm_duty_cycle_value	= static_cast<uint16_t>(req_dc_period_ns / pwm_prescaler_x_clk);

		std::cout << "Actual duty cycle will be " << +pwm_channels[channel].pwm_actual_duty_cycle  << std::endl;
		std::cout << "Duty cycle value was calculated as " << pwm_channels[channel].pwm_duty_cycle_value  << std::endl;
		std::cout << "                            in hex " << std::hex << pwm_channels[channel].pwm_duty_cycle_value  << std::endl;

	}; //1 000 000 000


	void PWM::PWM_Calc_ModuleInputs(int channel, int req_dc)
	{
		/* TODO move into try with custom exception */
		if(pwm_channels[channel].used){

			PWM_Calc_PeriodPrescaler(channel);
			PWM_Calc_ActualDutyCycle(channel, req_dc);

		} else{

			std::cerr << "Channel not initiated - please first initialize channel " << channel << std::endl;
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
		const uint8_t uint16_into_uin32 = 16; 
		uint32_t data_to_send;

		data_to_send = ((pwm_channels[channel].pwm_prescaler_value+1) << uint16_into_uin32 ) +  (pwm_channels[channel].pwm_period_value+1);
		 _PP_data = (etherbone::data_t) data_to_send;

		data_to_send = 0 +  pwm_channels[channel].pwm_duty_cycle_value;
		_DT_data = (etherbone::data_t) data_to_send;

		std::cout << "Filled _PP_data as " << std::hex << _PP_data << std::endl;
		std::cout << "Filled _DT_data as " << std::hex << _DT_data << std::endl;
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
		std::cout << "++++=======================================" << std::endl;
		std::cout << "PWM_Set_Channel called on " << channel  << std::endl;
		PWM_Calc_ActualFreq(channel, req_freq);
		PWM_Calc_ModuleInputs(channel, req_duty_cycle);
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