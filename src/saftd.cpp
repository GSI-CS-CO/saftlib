#include "saftd.hpp"
#include "saftbus.hpp"
#include "server_connection.hpp"

#include <iostream>

namespace mini_saftlib {

	SAFTd_Proxy::SAFTd_Proxy(SignalGroup &signal_group)
		: Proxy::Proxy("/de/gsi/saftlib", signal_group)
	{	
	}


	void SAFTd::greet() {
		std::cout << "hello from SAFTd" << std::endl;
	}


}