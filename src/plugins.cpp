/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#include "plugins.hpp"
#include "service.hpp"
#include "loop.hpp"

// #include <ltdl.h> // need libltdl for this
#include <dlfcn.h>

#include <cassert>
#include <iostream>
#include <sstream>
#include <set>

namespace saftbus {

	extern "C" typedef void (*create_services_function) (saftbus::Container *container, const std::vector<std::string> &args);

	struct LibraryLoader::Impl {
		// lt_dlhandle handle;
		void *handle;
		saftbus::Container *cont;
		create_services_function create_services;
	};

	LibraryLoader::LibraryLoader(const std::string &so_filename) 
		: d(new Impl)
	{
		d->cont = nullptr;
		int result = 0;//lt_dlinit();
		assert(result == 0);


		// d->handle = lt_dlopen(so_filename.c_str());
		d->handle = dlopen(so_filename.c_str(), RTLD_NOW|RTLD_GLOBAL);

		char *error = dlerror();
		if (d->handle == nullptr) {
			std::ostringstream msg;
			msg << "cannot load plugin: fail to open file " << so_filename;
			if (error) msg << " " << error;
			throw std::runtime_error(msg.str());
		} else {
			// //===std::cerr << "successfully opened " << so_filename << std::endl;
		}

		// load the function pointers
		// d->create_services = (create_services_function)lt_dlsym(d->handle,"create_services");
		d->create_services = (create_services_function)dlsym(d->handle,"create_services");
		if (d->create_services == nullptr) {
			// lt_dlclose(d->handle);
			dlclose(d->handle);
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
			// lt_dlclose(d->handle);
			dlclose(d->handle);
		}		
		int result = 0;//lt_dlexit();
		assert(result == 0);
		//===std::cerr << "lt_dlexit was successful" << std::endl;
	}

}