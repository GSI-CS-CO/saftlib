#ifndef MINI_SAFTLIB_SERVICE_
#define MINI_SAFTLIB_SERVICE_

#include "saftbus.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	class Service;

	// extern "C" {
	// 	typedef void (*service_call)(int interface_no, int function_no, Service *service, Deserializer &received, Serializer &send);
	// }

	class Service {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		Service(std::vector<std::string> interface_names);
		void call(Deserializer &received, Serializer &send);
		virtual ~Service();
		void add_signal_group(int fd);
	protected:
		virtual void call(unsigned interface_no, unsigned function_no, Deserializer &received, Serializer &send) = 0;
	};


	// this manages all other services (even itself)
	class CoreService : public Service {
		struct Impl; std::unique_ptr<Impl> d;
		static std::vector<std::string> gen_interface_names();
	public:
		CoreService();
		// static CoreService* get_instance();
		~CoreService();
		unsigned create_object(const std::string &object_path, std::unique_ptr<Service> service);
		unsigned register_proxy(const std::string &object_path, int client_fd, int signal_group_fd);
		// call a Service identified by the saftlib_object_id
		// return false if the saftlib_object_id is unknown
		bool call_service(unsigned saftlib_object_id, Deserializer &received, Serializer &send);

		void call(unsigned interface_no, unsigned function_no, Deserializer &received, Serializer &send);
	};


}

#endif
