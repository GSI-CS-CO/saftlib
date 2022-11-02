#include "plugins.hpp"
#include "make_unique.hpp"
#include "service.hpp"

#include <ltdl.h>

#include <cassert>
#include <iostream>
#include <set>

namespace saftbus {

	extern "C" typedef void (*create_services_function) (saftbus::Container *container, const std::vector<std::string> &args);

	struct LibraryLoader::Impl {
		lt_dlhandle handle;
		create_services_function create_services;
	};

	LibraryLoader::LibraryLoader(const std::string &so_filename) 
		: d(std2::make_unique<Impl>())
	{
		int result = lt_dlinit();
		assert(result == 0);


		d->handle = lt_dlopen(so_filename.c_str());
		if (d->handle == nullptr) {
			std::ostringstream msg;
			msg << "cannot load plugin: fail to open file " << so_filename;
			throw std::runtime_error(msg.str());
		} else {
			// std::cerr << "successfully opened " << so_filename << std::endl;
		}

		// load the function pointers
		d->create_services = (create_services_function)lt_dlsym(d->handle,"create_services");
		if (d->create_services == nullptr) {
			lt_dlclose(d->handle);
			throw std::runtime_error("cannot load plugin because symbol \"create_services\" cannot be loaded");
		}
	}

	void LibraryLoader::create_services(Container *container, const std::vector<std::string> &args) {
		d->create_services(container, args);
	}

	LibraryLoader::~LibraryLoader()
	{
		std::cerr << "~LibraryLoader()" << std::endl;

		if (d->handle != nullptr) {
			lt_dlclose(d->handle);
		}		
		int result = lt_dlexit();
		assert(result == 0);
		std::cerr << "lt_dlexit was successful" << std::endl;
	}

}