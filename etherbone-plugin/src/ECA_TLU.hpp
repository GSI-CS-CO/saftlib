#ifndef EB_PLUGIN_ECA_TLU_HPP_
#define EB_PLUGIN_ECA_TLU_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

namespace eb_plugin {

class ECA_TLU {
	etherbone::Device &device;
	eb_address_t eca_tlu;
public:
	ECA_TLU(etherbone::Device &device);
	void configInput(unsigned channel,
	                 bool enable,
	                 uint64_t event,
	                 uint32_t stable);
};

}

#endif
