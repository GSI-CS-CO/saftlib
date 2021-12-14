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

	// Client represents the program (running in another process)
	// that sent us a file descriptor of a socket pair.
	// The file descriptor itself serves as a unique id to identify this other process.
	struct Client {
		int socket_fd; // the file descriptor is a unique number and is used as a client id
		Client(int fd) : socket_fd(fd) {}
	};
	bool operator==(Client lhs, int rhs) // compare Clients by using their socket_fd
	{
		return lhs.socket_fd == rhs;
	}
	struct ServerConnection::Impl {
		Loop &loop;
		std::vector<Client> clients;
		Serializer   send;
		Deserializer received;
		Impl(Loop &l) 
			: loop(l) 
		{
			// make this big enough to avoid allocation in normal operation
			clients.reserve(1024);
		}
		bool accept_client(int fd, int condition);
		bool handle_client_request(int fd, int condition);
	};

	bool ServerConnection::Impl::accept_client(int fd, int condition) {
		if (condition & POLLIN) {
			int client_socket_fd = recvfd(fd);
			if (client_socket_fd == -1) {
				std::cerr << "cannot receive socket fd" << std::endl;
			}
			loop.connect(std::make_shared<IoSource>(sigc::mem_fun(*this, &ServerConnection::Impl::handle_client_request), client_socket_fd, POLLIN | POLLHUP));
			// register the client
			clients.push_back(Client(client_socket_fd));
			// send the ID back to client (the file descriptor integer number is used as ID)
			write(client_socket_fd, &client_socket_fd, sizeof(client_socket_fd));
		}
		return true;
	}

	bool ServerConnection::Impl::handle_client_request(int fd, int condition) {
		if (condition & POLLHUP) {
			std::cerr << "client hung up" << std::endl;
			std::cerr << "clients.size() " << clients.size() << std::endl;
			clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());
			close(fd);
			std::cerr << "clients.size() " << clients.size() << std::endl;
			return false;
		}
		if (condition & POLLIN) {
			bool result = received.read_from(fd);
			if (!result) {
				std::cerr << "failed to read data from fd " << fd << std::endl;
				return false;
			}
			MsgType type;
			received.get(type);
			switch(type) {
				case CALL: {
					std::cerr << "CALL request from client" << std::endl;
					int saftlib_object_id, interface;
					received.get(saftlib_object_id);
					received.get(interface);

				}
				break;
				case GET_SAFTLIB_OBJECT_ID: {
					std::cerr << "GET_SAFTLIB_OBJECT_ID received" << std::endl;
					std::string object_path;
					received.get(object_path);
					int signal_fd = recvfd(fd);
					// safe the signal_fd somewhere...
					std::cerr << "request from client " << fd << " wants the saftlib_object_id for " << object_path << std::endl;
					int saftlib_object_id = 42;
					send.put(saftlib_object_id);
					send.write_to(fd);
				}
				break;
			}

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