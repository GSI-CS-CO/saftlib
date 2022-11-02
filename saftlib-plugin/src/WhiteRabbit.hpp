#ifndef saftlib_WHITE_RABBIT_HPP_
#define saftlib_WHITE_RABBIT_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <functional>

namespace saftlib {

class WhiteRabbit {
	eb_address_t pps;
	etherbone::Device &device;
protected:
	mutable bool locked;
public:
	WhiteRabbit(etherbone::Device &device);
	std::function<void(bool locked)> SigLocked;
	/// @brief The timing receiver is locked to the timing grandmaster.
	/// @return The timing receiver is locked to the timing grandmaster.
	///
	/// Upon power-up it takes approximately one minute until the timing
	/// receiver has a correct timestamp.
	///
	// @saftbus-export
	bool getLocked() const;
};

}

#endif
