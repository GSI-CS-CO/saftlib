#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include "PluginLoader.h"

namespace saftlib {

	PluginLoader::PluginLoader(const std::string &filename)
		: handle_(0)
		, get_interface_names_(0)
	{
		// get handle to the shared object file

		dlerror();
		//std::cerr << "calling lt_dlopen(" << filename << ")" << std::endl;
		handle_ = dlopen(filename.c_str(), RTLD_NOW|RTLD_GLOBAL);
		if (!handle_)
		{
			throw std::runtime_error("cannot open shared library");
		}

		// get the four functions to create and destroy Source Unpacker and Processor objects

		// for Sources
		dlerror();
		get_interface_names_ = (get_interface_names_t*) dlsym(handle_, "get_interface_names");
		const char *dlsym_error = dlerror();
		if (!get_interface_names_ || dlsym_error)
		{
			std::ostringstream msg;
			msg << "error while getting symbol \"get_interface_names\": " << dlsym_error;
			throw std::runtime_error(msg.str().c_str());
		}


		std::vector<std::string> interface_names = get_interface_names_();
		for(auto name: interface_names) {
			std::cerr << name << std::endl;
		}
	}

	PluginLoader::~PluginLoader()
	{
		if (handle_)
		{
			dlclose(handle_);
		}
	}

}
