#ifndef MINI_SAFTLIB_SERVER_CONNECTION_
#define MINI_SAFTLIB_SERVER_CONNECTION_

#include "service.hpp"
#include "container.hpp"

#include <memory>
#include <string>

namespace mini_saftlib {

	class ServerConnection {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		ServerConnection(Container *container, const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ServerConnection();
	};

}


#endif
