#include "plugins.hpp"
#include "make_unique.hpp"
#include "service.hpp"

#include <ltdl.h>

#include <cassert>
#include <iostream>
#include <set>

namespace saftbus {

	extern "C" typedef std::vector<std::pair<std::string, std::unique_ptr<saftbus::Service> > > (*create_services_function) (saftbus::Container *container);
	extern "C" typedef void                                                                     (*destroy_service_function) (saftbus::Service *service);

	struct LibraryLoader::Impl {
		lt_dlhandle handle;
		create_services_function create_services;
		destroy_service_function destroy_service;
		std::set<saftbus::Service*> services;
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

		// load the function pointers
		d->create_services = (create_services_function)lt_dlsym(d->handle,"create_services");
		assert(d->create_services != NULL);
		d->destroy_service = (destroy_service_function)lt_dlsym(d->handle,"destroy_service");
		assert(d->destroy_service != NULL);
	}

	std::vector<std::pair<std::string, std::unique_ptr<Service> > > LibraryLoader::create_services(Container *container) {
		auto result = d->create_services(container);
		for (auto &name_service: result) {
			d->services.insert(name_service.second.get());
		}
		return result;
	}

	void LibraryLoader::destroy_service(Service *service) {
		std::cerr << "LibraryLoader::destroy_service " << std::endl;
		d->destroy_service(service);
		d->services.erase(service);
	}

	LibraryLoader::~LibraryLoader()
	{
		std::cerr << "~LibraryLoader()" << std::endl;

		if (d->handle != NULL) {
			for (auto &service: d->services) {
				d->destroy_service(service);
			}
			d->services.clear();
			lt_dlclose(d->handle);
		}		
		int result = lt_dlexit();
		assert(result == 0);
		std::cerr << "lt_dlexit was successful" << std::endl;
	}

}