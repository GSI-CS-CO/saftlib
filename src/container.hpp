#ifndef MINI_SAFTLIB_CONTAINER_
#define MINI_SAFTLIB_CONTAINER_

#include <memory>

namespace mini_saftlib {

	// Manage all Objects managed by mini-saftlib.
	// A typical daemon would have one instance of this class
	class Container {
		Container();

	private:
		struct Impl;
		std::unique_ptr<Impl> d;
	};

}


#endif