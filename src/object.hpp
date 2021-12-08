#ifndef MINI_SAFTLIB_OBJECT_
#define MINI_SAFTLIB_OBJECT_

#include <memory>

namespace mini_saftlib {

	class Object {
		Object();
	private:
		struct Impl;
		std::unique_ptr<Impl> d;
	};


}

#endif
