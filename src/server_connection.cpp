#include "server_connection.hpp"

#include <saftbus.hpp>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sigc++/sigc++.h>

namespace mini_saftlib {

	// Client represents the entity that sent us a file descritor
	// of a socket pair.
	// In our books it gets a unique id which is saved togther with that file descriptor 
	struct Client {
		int socket_fd;
		int client_id;
		Client(int fd, int id) : socket_fd(fd), client_id(id) {}
	};
	bool operator==(Client lhs, int rhs) // compare Clients by using the socket_fd
	{
		return lhs.socket_fd == rhs;
	}
	struct ServerConnection::Impl {
		Loop &loop;
		int client_id_generator;
		std::vector<Client> clients;
		Impl(Loop &l) 
			: loop(l) 
			, client_id_generator(0)
		{
			// make this big enough to avoid allocation in normal operation
			clients.reserve(1024);
		}
		bool accept_client(int fd, int condition);
		bool accept_call_from_client(int fd, int condition);
	};

	bool ServerConnection::Impl::accept_client(int fd, int condition) {
		if (condition & POLLIN) {
			int client_socket_fd = recvfd(fd);
			if (client_socket_fd == -1) {
				std::cerr << "cannot receive socket fd" << std::endl;
			}
			loop.connect(std::make_shared<IoSource>(sigc::mem_fun(*this, &ServerConnection::Impl::accept_call_from_client), client_socket_fd, POLLIN | POLLHUP));
			// give the client a unique ID.
			clients.push_back(Client(client_socket_fd, client_id_generator));
			// send the ID back to client
			write(client_socket_fd, &client_id_generator, sizeof(client_id_generator));
			++client_id_generator;
		}
		return true;
	}

	bool ServerConnection::Impl::accept_call_from_client(int fd, int condition) {
		if (condition & POLLHUP) {
			std::cerr << "client hung up" << std::endl;
			std::cerr << "clients.size() " << clients.size() << std::endl;
			clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());
			close(fd);
			std::cerr << "clients.size() " << clients.size() << std::endl;
			return false;
		}
		if (condition & POLLIN) {
			char ch;
			read(fd, &ch, 1);
			++ch;
			write(fd, &ch, 1);
		}
		return true;
	}










	ServerConnection::ServerConnection(Loop &loop, const std::string &socket_name) 
		: d(std::make_unique<Impl>(loop))
	{
		std::ostringstream msg;
		msg << "ServerConnection constructor : ";
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
		std::string dirname = socketname.substr(0,socketname.find_last_of('/'));
		std::ostringstream command;
		if (mkdir(dirname.c_str(), 0777)) {
			if (errno != EEXIST) {
				msg << "cannot create socket directory: " << dirname;
				throw std::runtime_error(msg.str());
			}
		}
		int unlink_result = unlink(socketname.c_str());
		if (unlink_result != 0) {
			msg << "could not unlink socket file " << socketname << ": " << strerror(errno);
		}
		struct sockaddr_un listen_sockaddr_un = {.sun_family = AF_LOCAL};
		strcpy(listen_sockaddr_un.sun_path, socketname.c_str());
		int bind_result = bind(base_socket_fd, (struct sockaddr*)&listen_sockaddr_un, sizeof(listen_sockaddr_un));
		if (bind_result != 0) {
			msg << "could not bind to socket: " << strerror(errno);
			throw std::runtime_error(msg.str());
		}
		chmod(socketname.c_str(), S_IRUSR | S_IWUSR | 
			                      S_IRGRP | S_IWGRP | 
			                      S_IROTH | S_IWOTH );
		chmod(dirname.c_str(), S_IRUSR | S_IWUSR | 
			                   S_IRGRP | S_IWGRP | 
			                   S_IROTH | S_IWOTH );

		loop.connect(std::make_shared<IoSource>(sigc::mem_fun(*d, &ServerConnection::Impl::accept_client), base_socket_fd, POLLIN));
		// Slib::signal_io().connect(sigc::mem_fun(*this, &Connection::accept_client), base_socket_fd, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_LOW);			
	}

	ServerConnection::~ServerConnection() = default;

}