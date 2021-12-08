#include "container.hpp"

#include <map>
#include <string>

namespace mini_saftlib {

	struct Container::Impl {
		// every object is created with an object path. It also gets a unique id with saftlib.
		// This map allows to find the saftid using the object path string.
		std::map<std::string, int> objpath_to_saftid; 

		// this is used to generate unique saftlib id. it is incremented whenever an object is created
		int saftid_counter;
	};

	Container::Container() 
		: d(std::make_unique<Impl>())
	{

	}



}