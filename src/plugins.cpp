#include "plugins.hpp"
#include "make_unique.hpp"

#include <ltdl.h>

#include <cassert>
#include <iostream>

namespace mini_saftlib {

	struct LibraryLoader::Impl {
		lt_dlhandle handle;
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

	}

	LibraryLoader::~LibraryLoader() = default;

}