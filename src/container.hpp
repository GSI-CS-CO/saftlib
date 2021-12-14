#ifndef MINI_SAFTLIB_CONTAINER_
#define MINI_SAFTLIB_CONTAINER_

#include "saftbus.hpp"

#include <memory>

namespace mini_saftlib {

	// Manage all Objects managed by mini-saftlib.
	// A typical daemon would have one instance of this class
	class Container {
		Container();
		void remote_call(int saftlib_object_id, int interface, Deserializer &received);
	private:
		struct Impl;
		std::unique_ptr<Impl> d;
	};

}


#endif