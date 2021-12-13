#ifndef MINI_SAFTLIB_CLIENT_CONNECTION_
#define MINI_SAFTLIB_CLIENT_CONNECTION_

#include <saftbus.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	class ClientConnection {
	public:
		ClientConnection(const std::string &socket_name = "/var/run/mini-saftlib/saftbus");
		~ClientConnection();

		// buffer to serialize arguments and de-serialize return values
		SerDes serdes; 
		// send whatever data is in serial buffer to the server
		bool send(int saftlib_object_id, int interface_id, int timeout_ms = -1); 
		// wait for data to arrive from the server
		bool receive(int timeout_ms = -1);

		void send_call();
		void send_disconnect();

	private:
		struct Impl; std::unique_ptr<Impl> d;		
	};

}

#endif
