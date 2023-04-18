/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
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

#ifndef saftlib_IO_HPP_
#define saftlib_IO_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <functional>
#include <string>

namespace saftlib {

class SerdesClockGen;

class Io {
	etherbone::Device &device;
	std::string io_name;
	unsigned io_direction;
	unsigned io_channel;
	unsigned io_eca_in;
	unsigned io_eca_out;
	unsigned io_index;
	unsigned io_special_purpose;
	unsigned io_logic_level;
	bool io_oe_available;
	bool io_term_available;
	bool io_spec_out_available;
	bool io_spec_in_available;
	eb_address_t io_control_addr;
	SerdesClockGen &io_clkgen;
public:
	Io(etherbone::Device &device
	   , const std::string &io_name
	   , unsigned io_direction
	   , unsigned io_channel
	   , unsigned eca_in
	   , unsigned eca_out
	   , unsigned io_index
	   , unsigned io_special_purpose
	   , unsigned io_logic_level
	   , bool io_oe_available
	   , bool io_term_available
	   , bool io_spec_out_available
	   , bool io_spec_in_available
	   , eb_address_t io_control_addr
	   , SerdesClockGen &clkgen
	);



	const std::string &getName() const;
	unsigned getDirection() const;
	// iOutputActionSink
	uint32_t getIndexOut() const;
	uint32_t getIndexIn() const;
	uint32_t getEcaIn() const;
	uint32_t getEcaOut() const;
	void WriteOutput(bool value);
	bool ReadOutput();
	bool ReadCombinedOutput();
	bool getOutputEnable() const;
	void setOutputEnable(bool val);
	bool getOutputEnableAvailable() const;
	bool getSpecialPurposeOut() const;
	void setSpecialPurposeOut(bool val);
	bool getSpecialPurposeOutAvailable() const;
	bool getGateOut() const;
	void setGateOut(bool val);
	bool getBuTiSMultiplexer() const;
	void setBuTiSMultiplexer(bool val);
	bool getPPSMultiplexer() const;
	void setPPSMultiplexer(bool val);
	bool StartClock(double high_phase, double low_phase, uint64_t phase_offset);
	bool StopClock();
	std::string getLogicLevelOut() const;
	std::string getTypeOut() const;

	// iInputEventSource
	bool ReadInput(); // done
	bool getInputTermination() const;
	void setInputTermination(bool val);
	bool getInputTerminationAvailable() const;
	bool getSpecialPurposeIn() const;
	void setSpecialPurposeIn(bool val);
	bool getSpecialPurposeInAvailable() const;
	bool getGateIn() const;
	void setGateIn(bool val);
	std::string getLogicLevelIn() const;
	std::string getTypeIn() const;

	// iInputEventSource
	uint64_t getResolution() const;

	bool ConfigureClock(double high_phase, double low_phase, uint64_t phase_offset);
	std::string getLogicLevel() const;
	std::string getType() const;


	std::function< void(bool) > OutputEnable;
	std::function< void(bool) > SpecialPurposeOut;
	std::function< void(bool) > GateOut;
	std::function< void(bool) > BuTiSMultiplexer;
	std::function< void(bool) > PPSMultiplexer;
	std::function< void(bool) > InputTermination;
	std::function< void(bool) > SpecialPurposeIn;
	std::function< void(bool) > GateIn;

};

}

#endif
