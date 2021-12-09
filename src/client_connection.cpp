#include "client_connection.hpp"

#include <saftbus.hpp>

#include <sstream>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace mini_saftlib {

	struct ClientConnection::Impl {
		int socket_fd;
		int client_id;
	};


	ClientConnection::ClientConnection(const std::string &socket_name) 
		: d(std::make_unique<Impl>())
	{
		std::ostringstream msg;
		msg << "ClientConnection constructor : ";
		char *socketname_env = getenv("SAFTBUS_SOCKET_PATH");
		std::string socketname = socket_name;
		if (socketname_env != nullptr) {
			socketname = socketname_env;
		}
		if (socketname.size() == 0) {
			msg << "invalid socket name (name is empty)";
			throw std::runtime_error(msg.str());
		}
		if (socketname[0] != '/') {
			msg << "saftbus socketname " << socketname << " is not an absolute pathname";
			throw std::runtime_error(msg.str());
		}
		int base_socket_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if (base_socket_fd <= 0) {
			msg << " cannot create socket: " << strerror(errno);
			throw std::runtime_error(msg.str());
		}
		struct sockaddr_un call_sockaddr_un = {.sun_family = AF_LOCAL};
		strcpy(call_sockaddr_un.sun_path, socketname.c_str());

		int connect_result = connect(base_socket_fd, (struct sockaddr *)&call_sockaddr_un , sizeof(call_sockaddr_un));
		if (connect_result != 0) {
			msg << "cannot connect to socket: " << socketname << ". Possible reasons: server not running, wrong socket path (set SAFTBUS_SOCKET_PATH evironment variable), or wrong permissions";
			throw std::runtime_error(msg.str());
		}

		int fd_pair[2];
		if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, fd_pair) != 0) {
			msg << "cannot create socket pair: " << strerror(errno);
			throw std::runtime_error(msg.str());
		}
		if (sendfd(base_socket_fd, fd_pair[0]) == -1) {
			msg << "cannot send socket pair: " << strerror(errno);
			throw std::runtime_error(msg.str());
		}
		close(fd_pair[0]);
		d->socket_fd = fd_pair[1];

		if (read(d->socket_fd, &d->client_id, sizeof(d->client_id)) != sizeof(d->client_id)) {
			msg << "cannot read client id" << strerror(errno) << std::endl;
			throw std::runtime_error(msg.str());
		}
		std::cerr << "got client id " << d->client_id << std::endl;
		char ch = '1';
		std::cerr << "write " << write(d->socket_fd, &ch, sizeof(ch)) << std::endl;
		std::cerr << "read "  <<  read(d->socket_fd, &ch, sizeof(ch)) << std::endl;
		std::cerr << "got " << ch << std::endl;


		sleep(10);
		// saftbus::write(get_fd(), saftbus::SENDER_ID);  // ask the saftd for an ID on the saftbus
		// saftbus::read(get_fd(), _saftbus_id);  
	}

	ClientConnection::~ClientConnection() {
		close(d->socket_fd);
	}


}