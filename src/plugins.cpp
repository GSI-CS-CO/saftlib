#include "plugins.hpp"
#include "make_unique.hpp"

namespace mini_saftlib {

	struct LibraryLoader::Impl {

	};

	LibraryLoader::LibraryLoader(const std::string &so_filename) 
		: d(std2::make_unique<Impl>())
	{

	}

	LibraryLoader::~LibraryLoader() = default;

}