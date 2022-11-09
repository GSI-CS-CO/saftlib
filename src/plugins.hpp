#ifndef SAFTBUS_PLUGINS_HPP_
#define SAFTBUS_PLUGINS_HPP_

#include "service.hpp"
#include "loop.hpp"

#include <string>
#include <vector>
#include <map>

namespace saftbus {

	class LibraryLoader {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		LibraryLoader(const std::string &so_filename);
		~LibraryLoader();
		// the service objects are owned by the plugin, we only get a naked pointer to it
		// the std::string is the object path under which the respective Servie object should be mounted		
		void create_services(Container *container, const std::vector<std::string> &args = std::vector<std::string>()); 
	};

}



#endif
