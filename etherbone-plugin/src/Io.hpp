#ifndef EB_PLUGIN_IO_HPP_
#define EB_PLUGIN_IO_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <functional>

namespace eb_plugin {

class Io {
	etherbone::Device &device;
	unsigned io_channel;
	unsigned io_index;
	unsigned io_special_purpose;
	unsigned io_logic_level;
	bool io_oe_available;
	bool io_term_available;
	bool io_spec_out_available;
	bool io_spec_in_available;
	eb_address_t io_control_addr;
public:
	Io(etherbone::Device &device
	   , unsigned io_channel
	   , unsigned io_index
	   , unsigned io_special_purpose
	   , unsigned io_logic_level
	   , bool io_oe_available
	   , bool io_term_available
	   , bool io_spec_out_available
	   , bool io_spec_in_available
	   , eb_address_t io_control_addr
	);



	// iOutputActionSink
	uint32_t getIndexOut() const;
	uint32_t getIndexIn() const;
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
