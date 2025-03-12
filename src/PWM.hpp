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

#ifndef saftlib_PWM_SDB_HPP_
#define saftlib_PWM_SDB_HPP_

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <vector>

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#endif

#define PWM_PRODUCT_ID        0x434E5453
#define PWM_VENDOR_ID         0x0000000000000651

#define PWM_MAX_CHANNEL  	8

#define PWM_TEST_PERIOD    	0x9C3F
#define PWM_TEST_PRESCALER  0x03E7
#define PWM_TEST_PP			0x9CEF03E7
#define PWM_TEST_DUTYCYCLE  0x00004E1F

#define PWM_NR_CH_OFFSET  	0x04
#define PWM_DT_CH_0_OFFSET  0x08
#define PWM_DT_CH_1_OFFSET  0x0C
#define PWM_DT_CH_2_OFFSET  0x10
#define PWM_DT_CH_3_OFFSET  0x14
#define PWM_DT_CH_4_OFFSET  0x18
#define PWM_DT_CH_5_OFFSET  0x1C
#define PWM_DT_CH_6_OFFSET  0x20
#define PWM_DT_CH_7_OFFSET  0x24

/* all parameters of the PWM depend on clock cycle time */
/* current ref clock has 62.5 Mhz -> period of 16ns */
#define PWM_REF_CLK_TIME_NS		16
#define PWM_REF_CLK_FREQ_HZ		62500000
#define PWM_REF_MIN_PERIOD_NS	240
#define PWM_REF_MAX_FREQ_HZ		4167000
#define PWM_S_TO_NS 			1000000000
#define PWM_MIN_PRESCALER 		2
#define PWM_MAX_MOD_VALUE		0xFFFF

#include <etherbone.h>

#include "SdbDevice.hpp"

namespace saftlib {


class PWM : public SdbDevice {

public:
	PWM(etherbone::Device &device);
	eb_address_t get_start_adr();

	//constexpr int maxFreq() { return 10; }
	void PWM_Set_Test_PP(void);
	void PWM_Set_Test_DC(etherbone::address_t pwm_target_reg, etherbone::data_t data_to_send);
	void PWM_Calc_ActualFreq(int channel, int req_freq);
	void PWM_Calc_ActualFreq(uint8_t channel, uint32_t req_freq);
	void PWM_Calc_ActualDutyCycle(int channel, int req_dc);
	void PWM_Calc_PeriodPrescaler(int channel);
	void PWM_Calc_ModuleInputs(int channel, int req_dc);
	etherbone::address_t PWM_Get_ChannelRegAdr(int channel);
	std::vector<long> PWM_Calc_Factors(long num_to_factorise);
	void PWM_Pack_Data(int channel);
	void PWM_Send_Config(int channel);
	// @saftbus-export
	void PWM_Test(int channel);
	// @saftbus-export
	void PWM_Set_Channel(int channel, int req_freq, int req_duty_cycle);
	

private:

	/* this address is for all instances*/
	etherbone::address_t _pwm_addr_0;

	/* this array is equal for all instances*/
	struct pwm_channel_t {

		bool 		used;
		double 		pwm_freq_double;
		long 		pwm_period_long;
		double 		pwm_period_counter;
		uint16_t 	pwm_period_value;
		uint16_t 	pwm_prescaler_value;
		double 		pwm_actual_duty_cycle;
		uint16_t 	pwm_duty_cycle_value;

	} pwm_channels[PWM_MAX_CHANNEL -1];

	void PWM_Set_All(int channel);

	etherbone::data_t _PP_data;
	etherbone::data_t _DT_data;
	etherbone::Cycle _cycle;
	etherbone::exception_t _e;
};

}

#endif
