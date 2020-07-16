#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include <Connection.h>

#include "PluginLoader.h"

namespace saftbus {

	PluginLoader::PluginLoader(const std::string &filename, const std::shared_ptr<Slib::MainContext> &context, saftbus::Connection &connection)
		: handle_(nullptr)
		, initialize_(nullptr)
		, cleanup_(nullptr)
	{
		// get handle to the shared object file
		dlerror();
		handle_ = dlopen(filename.c_str(), RTLD_NOW | RTLD_GLOBAL );
		if (!handle_)
		{
			throw std::runtime_error((filename + ": cannot open shared library").c_str());
		}

		const char *dlsym_error;
		// load function that can create a driver instance
		dlerror();
		initialize_ = (initialize_t*) dlsym(handle_, "initialize");
		dlsym_error = dlerror();
		if (!initialize_ || dlsym_error)
		{
			std::ostringstream msg;
			msg << "error while getting symbol \"initialize\": " << dlsym_error;
			throw std::runtime_error(msg.str().c_str());
		}

		// load function that can destroy a driver instance
		dlerror();
		cleanup_ = (cleanup_t*) dlsym(handle_, "cleanup");
		dlsym_error = dlerror();
		if (!cleanup_ || dlsym_error)
		{
			std::ostringstream msg;
			msg << "error while getting symbol \"cleanup\": " << dlsym_error;
			throw std::runtime_error(msg.str().c_str());
		}


		// got all function pointers. initialize the plugin
		std::cerr << "call initialize_(context)" << std::endl;
		initialize_(context, connection);
	}

	PluginLoader::~PluginLoader()
	{
		if (handle_)
		{
			cleanup_();

			//dlclose(handle_);
			// dlclose(handle_) should be called to clean up... but if the plugin registered a timeout that 
			// is active and is scheduled for execution in the future, then MainContext will notice 
			// that the timeout was removed will remove the sigc::slot. The sigc::~slot destructor 
			// has somehow connections into the plugin and gets a segfault when the handle_ was 
			// already destroyed.
			// I consider this is as noncritical, since plugins will only be loaded on startup and then 
			// normally never unloaded. So there is no actual resource leak here in practice.
		}
	}

}
