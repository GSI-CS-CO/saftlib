#include "service.hpp"

#include "make_unique.hpp"
#include "plugins.hpp"
#include "loop.hpp"

#include <string>
#include <map>
#include <set>
#include <cassert>

#include <unistd.h>

namespace saftbus {

	struct Service::Impl {
	};

	struct Container::Impl {
	};

	struct Container_Service::Impl {
	};


	Service::Service(const std::vector<std::string> &interface_names) 
	{
	}
	Service::~Service() = default;

	// Generate the mapping from interface_name to interface_no for all interface_names,
	// return true if all interface_names are found, false otherwise
	bool Service::get_interface_name2no_map(const std::vector<std::string> &interface_names, std::map<std::string, int> &interface_name2no_map)
	{
		return false;
	}

	void Service::call(int client_fd, Deserializer &received, Serializer &send) {
	}

	void Service::emit(Serializer &send)
	{
	}

	int Service::get_object_id() 
	{
		return 0;
	}

	std::string &Service::get_object_path()
	{
		static std::string str("");
		return str;
	}
	std::vector<std::string> &Service::get_interface_names() {
		static std::vector<std::string> result;
		return result;
	}




	Container_Service::Container_Service(Container *container)
		: Service(get_interface_names())
	{
	}
	Container_Service::~Container_Service() = default;

	bool Container_Service::emit_periodical_signal() {
		return false;
	}

	void Container_Service::call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) {
	}






	Container::Container(ServerConnection *connection) 
	{
	}

	Container::~Container() = default;

	unsigned Container::create_object(const std::string &object_path, std::unique_ptr<Service> service)
	{
		return 0;
	}

	bool Container::remove_object(const std::string &object_path)
	{
		return false;
	}

	int Container::register_proxy(const std::string &object_path, const std::vector<std::string> interface_names, std::map<std::string, int> &interface_name2no_map, int client_fd, int signal_group_fd)
	{
		return 0;
	}
	void Container::unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd)
	{
	}


	bool Container::call_service(unsigned saftlib_object_id, int client_fd, Deserializer &received, Serializer &send) {
		return false;
	}

	void Container::remove_signal_fd(int fd)
	{
	}

	void Container::clear() {
	}


}
