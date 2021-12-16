#ifndef MINI_SAFTLIB_CONTAINER_
#define MINI_SAFTLIB_CONTAINER_

#include "saftbus.hpp"
#include "service.hpp"

#include <memory>

namespace mini_saftlib {

	// Manage all Objects managed by mini-saftlib.
	class Container {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		Container();
		~Container();
		// insert an object and return the saftlib_object_id for this object
		// return 0 in case of failure
		unsigned create_object(const std::string &object_path, std::unique_ptr<Service> service);
		// return saftlib_object_id if the object_path was found, 0 otherwise
		unsigned register_proxy(const std::string &object_path, int client_fd, int signal_group_fd);
		void unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd);
		// call a Service identified by the saftlib_object_id
		// return false if the saftlib_object_id is unknown
		bool call_service(unsigned saftlib_object_id, Deserializer &received, Serializer &send);
	};

}


#endif