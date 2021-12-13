#ifndef MINI_SAFTLIB_SERVER_CONNECTION_
#define MINI_SAFTLIB_SERVER_CONNECTION_

#include <memory>
#include <string>

#include "loop.hpp"

namespace mini_saftlib {

	class ServerConnection {
	public:
		enum MsgType {
			CALL,
			DISCONNECT
		};
		ServerConnection(Loop &loop, const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ServerConnection();
	private:
		struct Impl; std::unique_ptr<Impl> d;
	};

}


#endif
