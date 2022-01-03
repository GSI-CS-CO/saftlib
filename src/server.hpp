#ifndef MINI_SAFTLIB_SERVER_CONNECTION_
#define MINI_SAFTLIB_SERVER_CONNECTION_


#include <memory>
#include <string>

namespace mini_saftlib {

	class ServerConnection {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		ServerConnection(const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ServerConnection();

		void register_signal_id_for_client(int client_id, int signal_id);
		void unregister_signal_id_for_client(int client_id, int signal_id);
	};

}


#endif
