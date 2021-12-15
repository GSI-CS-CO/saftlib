#ifndef MINI_SAFTLIB_SERVICE_
#define MINI_SAFTLIB_SERVICE_

#include "saftbus.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	class Service;

	extern "C" {
		typedef void (*service_call)(int interface_no, int function_no, Service *service, Deserializer &received, Serializer &send);
	}

	class Service {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		Service(std::vector<std::string> interface_names, service_call call);
		void call(Deserializer &received, Serializer &send);
		virtual ~Service();
		void add_signal_group(int fd);
	};


}

#endif
