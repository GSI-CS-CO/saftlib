#include "plugins.hpp"
#include "make_unique.hpp"
#include "service.hpp"

#include <ltdl.h>

#include <cassert>
#include <iostream>

namespace saftbus {

	extern "C" typedef std::string              (*get_object_path_function)(Container *container);
	extern "C" typedef std::unique_ptr<Service> (*create_service_function) (Container *container);

	struct LibraryLoader::Impl {
		lt_dlhandle handle;
		get_object_path_function get_object_path;
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

		// get the create_service function
		d->get_object_path = (get_object_path_function)lt_dlsym(d->handle,"get_object_path");
		assert(d->get_object_path != NULL);
		d->create_service = (create_service_function)lt_dlsym(d->handle,"create_service");
		assert(d->create_service != NULL);
	}

	std::string LibraryLoader::get_object_path(Container *container) {
		return d->get_object_path(container);
	}

	std::unique_ptr<Service> LibraryLoader::create_service(Container *container) {
		return d->create_service(container);
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