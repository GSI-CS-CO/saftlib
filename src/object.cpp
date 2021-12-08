#include "object.hpp"

namespace mini_saftlib {

	struct Object::Impl {
		int Owner;
	};

	Object::Object() 
		: d(std::make_unique<Impl>())
	{}

}