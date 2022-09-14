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
		std::vector<std::pair<std::string, std::unique_ptr<Service> > > create_services(Container *container); 

		// we are done with the Service object, tell the plugin that it is safe to destroy it
		void destroy_service(Service *service); 
	};

}



#endif
