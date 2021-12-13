#include "object.hpp"

#include <string>

namespace mini_saftlib {

	class Service;

	struct Object::Impl {
		int         owner;
		std::string object_path;
		std::string interface_name;
	};

	Object::Object() 
		: d(std::make_unique<Impl>())
	{}
	Object::~Object() = default;

}
