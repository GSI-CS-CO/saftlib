#include "plugins.hpp"
#include "make_unique.hpp"
#include "service.hpp"

#include <ltdl.h>

#include <cassert>
#include <iostream>

namespace mini_saftlib {

	extern "C" typedef std::unique_ptr<Service> (*create_service_function)();

	struct LibraryLoader::Impl {
		lt_dlhandle handle;
		create_service_function create_service;
	};

	LibraryLoader::LibraryLoader(const std::string &so_filename) 
		: d(std2::make_unique<Impl>())
	{
		int result = lt_dlinit();
		assert(result == 0);
		std::cerr << "lt_dlinit was successful" << std::endl;


		d->handle = lt_dlopen(so_filename.c_str());
		if (d->handle == NULL) {
			std::cerr << "fail to open so file: " << so_filename << std::endl;
		} else {
			std::cerr << "successfully opened " << so_filename << std::endl;
		}

		// // get the create_service function
		d->create_service = (create_service_function)lt_dlsym(d->handle,"create_service");
		assert(d->create_service != NULL);
	}
	std::unique_ptr<Service> LibraryLoader::create_service() {
		return d->create_service();
	}

	LibraryLoader::~LibraryLoader()
	{
		if (d->handle != NULL) {
			lt_dlclose(d->handle);
		}		
		int result = lt_dlexit();
		assert(result == 0);
		std::cerr << "lt_dlexit was successful" << std::endl;
	}

}