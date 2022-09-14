#ifndef SAFTBUS_SERVICE_HPP_
#define SAFTBUS_SERVICE_HPP_

#include "saftbus.hpp"
#include "server.hpp"

#include <memory>
#include <string>
#include <vector>

namespace saftbus {

	// base class of all Services
	class Service {
		struct Impl; std::unique_ptr<Impl> d;
		friend class Container;
		friend class Container_Service;
	public:
		Service(const std::vector<std::string> &interface_names);
		bool get_interface_name2no_map(const std::vector<std::string> &interface_names, std::map<std::string, int> &interface_name2no_map);
		void call(int client_fd, Deserializer &received, Serializer &send);
		virtual ~Service();
	protected:
		virtual void call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) = 0;
		void emit(Serializer &send);
		int get_object_id();
		std::string &get_object_path();
		std::vector<std::string> &get_interface_names();
		Container *get_container();
	};



	// Container of all Services provided by mini-saftlib.
	class Container {
		struct Impl; std::unique_ptr<Impl> d;
		friend class Container_Service;
	public:
		Container(ServerConnection *connection);
		~Container();
		// insert an object and return the saftlib_object_id for this object
		// return 0 in case the object_path is unknown
		unsigned create_object(const std::string &object_path, std::unique_ptr<Service> service);
		bool remove_object(const std::string &object_path);
		bool remove_object_delayed(const std::string &object_path);
		// return saftlib_object_id if the object_path was found and all requested interfaces are implemented
		// return 0 if object_path was not found
		// return -1 if object_path was found but not all requested interfaces are implmented by the object
		int register_proxy(const std::string &object_path, const std::vector<std::string> interface_names, std::map<std::string, int> &interface_name2no_map, int client_fd, int signal_group_fd);
		void unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd);
		// call a Service identified by the saftlib_object_id
		// return false if the saftlib_object_id is unknown
		bool call_service(unsigned saftlib_object_id, int client_fd, Deserializer &received, Serializer &send);
		void remove_signal_fd(int fd);


		bool load_plugin(const std::string &so_filename);
		void clear();
	};

	class ServerConnection;
	// A Service to access the Container of Services
	// mainly Proxy (de-)registration 
	class Container_Service : public Service {
		struct Impl; std::unique_ptr<Impl> d;
		bool emit_periodical_signal();
	public:
		Container_Service(Container *container);
		~Container_Service();
		void call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send);
	};

}

#endif
