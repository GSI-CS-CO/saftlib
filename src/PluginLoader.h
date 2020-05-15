#ifndef SAFTLIB_PLUGIN_LOADER_H_
#define SAFTLIB_PLUGIN_LOADER_H_


#include <vector>
#include <string>
#include <map>

#include <dlfcn.h>

namespace saftlib
{

	typedef std::vector<std::string> get_interface_names_t();

	class PluginLoader
	{
		public:
			PluginLoader(const std::string &filename);
			~PluginLoader();

		private:
			void * handle_;

			get_interface_names_t     *get_interface_names_;
	};


}


#endif
