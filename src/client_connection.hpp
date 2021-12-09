#ifndef MINI_SAFTLIB_CLIENT_CONNECTION_
#define MINI_SAFTLIB_CLIENT_CONNECTION_

#include <memory>
#include <string>

namespace mini_saftlib {

	class ClientConnection {
	public:
		ClientConnection(const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ClientConnection();
	private:
		struct Impl; std::unique_ptr<Impl> d;		
	};

}

#endif
