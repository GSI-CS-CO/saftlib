#ifndef MINI_SAFTLIB_SERVICE_
#define MINI_SAFTLIB_SERVICE_

#include "saftbus.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	class Service;
	class Container;

	class Service {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		Service(std::vector<std::string> interface_names);
		void call(int client_fd, Deserializer &received, Serializer &send);
		virtual ~Service();
		void add_signal_group(int fd);
	protected:
		virtual void call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) = 0;
	};


	// this Service manages Proxy (de-)registration 
	class CoreService : public Service {
		struct Impl; std::unique_ptr<Impl> d;
		static std::vector<std::string> gen_interface_names();
	public:
		CoreService(Container *container);
		~CoreService();
		void call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send);
	};

}

#endif
