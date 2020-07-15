#ifndef SAFTBUS_PLUGIN_LOADER_H_
#define SAFTBUS_PLUGIN_LOADER_H_


#include <vector>
#include <string>
#include <map>

#include <dlfcn.h>

#include <MainContext.h>
#include <core.h>

namespace saftbus
{

	typedef std::vector<std::string> get_interface_names_t();
	typedef void *                   create_driver_t(const std::shared_ptr<Slib::MainContext> &loop);
	typedef void                     destroy_driver_t(void *);
	typedef void                     method_call_t(const std::string &method_name, const saftbus::Serial &parameters, saftbus::Serial &response);
	typedef void                     property_get_t(const std::string &property_name, saftbus::Serial &value);
	typedef void                     property_set_t(const std::string &property_name, const saftbus::Serial &value);



	class PluginLoader
	{
		public:
			PluginLoader(const std::string &filename, const std::shared_ptr<Slib::MainContext> &context);
			~PluginLoader();

		private:
			void * handle_; // handle to the shared object

			get_interface_names_t     *get_interface_names_;
			create_driver_t           *create_driver_;
			destroy_driver_t          *destroy_driver_;
			method_call_t			  *method_call_;
			property_get_t			  *property_get_;
			property_set_t			  *property_set_;

			void *driver_instance;


	};


}


#endif
