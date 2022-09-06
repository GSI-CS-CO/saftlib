#ifndef SAFTBUS_PLUGINS_HPP_
#define SAFTBUS_PLUGINS_HPP_

#include "service.hpp"
#include "loop.hpp"

#include <string>
#include <memory>

namespace saftbus {

	class LibraryLoader {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		LibraryLoader(const std::string &so_filename);
		~LibraryLoader();
		std::string              get_object_path(Container *container);
		std::unique_ptr<Service> create_service (Container *container);
	};

}



#endif
