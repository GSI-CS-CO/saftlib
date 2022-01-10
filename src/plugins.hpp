#ifndef MINI_SAFTLIB_PLUGINS_
#define MINI_SAFTLIB_PLUGINS_

#include "service.hpp"
#include "loop.hpp"

#include <string>
#include <memory>

namespace mini_saftlib {

	class LibraryLoader {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		LibraryLoader(const std::string &so_filename);
		~LibraryLoader();
		std::unique_ptr<Service> create_service();
	};

}



#endif
