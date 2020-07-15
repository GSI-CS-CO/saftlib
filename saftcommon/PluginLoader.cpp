#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include "PluginLoader.h"

namespace saftbus {

	PluginLoader::PluginLoader(const std::string &filename, const std::shared_ptr<Slib::MainContext> &context)
		: handle_(0)
		, get_interface_names_(0)
	{
		// get handle to the shared object file


		dlerror();
		handle_ = dlopen(filename.c_str(), RTLD_NOW|RTLD_GLOBAL);
		if (!handle_)
		{
			throw std::runtime_error((filename + ": cannot open shared library").c_str());
		}

		// get the functions to create and destroy Source Unpacker and Processor objects

		// load function that gives driver interface information
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


		// load function that can create a driver instance
		dlerror();
		create_driver_ = (create_driver_t*) dlsym(handle_, "create_driver");
		dlsym_error = dlerror();
		if (!get_interface_names_ || dlsym_error)
		{
			std::ostringstream msg;
			msg << "error while getting symbol \"create_driver\": " << dlsym_error;
			throw std::runtime_error(msg.str().c_str());
		}
		std::cerr << "call create_driver_(context)" << std::endl;
		driver_instance = create_driver_(context);

		// load function that can destroy a driver instance
		dlerror();
		destroy_driver_ = (destroy_driver_t*) dlsym(handle_, "destroy_driver");
		dlsym_error = dlerror();
		if (!get_interface_names_ || dlsym_error)
		{
			std::ostringstream msg;
			msg << "error while getting symbol \"destroy_driver\": " << dlsym_error;
			throw std::runtime_error(msg.str().c_str());
		}

		// load function that can call a method
		dlerror();
		method_call_ = (method_call_t*) dlsym(handle_, "method_call");
		dlsym_error = dlerror();
		if (!get_interface_names_ || dlsym_error)
		{
			std::ostringstream msg;
			msg << "error while getting symbol \"method_call\": " << dlsym_error;
			throw std::runtime_error(msg.str().c_str());
		}


	}

	PluginLoader::~PluginLoader()
	{
		if (handle_)
		{
			destroy_driver_(driver_instance);
			dlclose(handle_);
		}
	}

}
