#ifndef EB_PLUGIN_ECA_TLU_HPP_
#define EB_PLUGIN_ECA_TLU_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <memory>
#include <map>

namespace eb_plugin {

class Input;

class ECA_TLU {
	etherbone::Device &device;
	eb_address_t eca_tlu;

	std::vector<std::unique_ptr<Input> > inputs;

public:
	ECA_TLU(etherbone::Device &device);

	/// @brief add source and let ECA take ownership of the sink object
	void addInput(std::unique_ptr<Input> input);

	// @saftbus-export
	std::map< std::string, std::string > getInputs() const;

	void configInput(unsigned channel,
	                 bool enable,
	                 uint64_t event,
	                 uint32_t stable);
};

}

#endif
