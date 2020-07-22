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
	class Connection; 


	typedef std::vector<std::string> get_interface_names_t();
	typedef void                     initialize_t(const std::shared_ptr<Slib::MainContext> &loop, 
		                                          const std::shared_ptr<saftbus::Connection> &connection);
	typedef void                     cleanup_t();




	class PluginLoader
	{
		public:
			PluginLoader(const std::string &filename, 
				         const std::shared_ptr<Slib::MainContext> &context, 
				         const std::shared_ptr<saftbus::Connection> &connection);
			~PluginLoader();

		private:
			void * handle_; // handle to the shared object

			initialize_t              *initialize_;
			cleanup_t                 *cleanup_;
	};


}


#endif
