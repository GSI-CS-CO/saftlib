#include "saftd.hpp"

namespace mini_saftlib {

	SAFTd_Proxy::SAFTd_Proxy(SignalGroup &signal_group)
		: Proxy::Proxy("/de/gsi/saftlib", signal_group)
	{	
	}

	// void SAFTd_Proxy::greet() {

	// }



}