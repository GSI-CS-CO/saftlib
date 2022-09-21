#include "server.hpp"
#include "service.hpp"
#include "saftbus.hpp"
#include "loop.hpp"
#include "make_unique.hpp"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>
#include <set>
#include <map>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace saftbus {


	// Client represents the program (running in another process)
	// that sent us one file descriptor of a socket pair.
	// The file descriptor integer value serves as a unique id to identify this other process.
	struct Client {
		int socket_fd; // the file descriptor is a unique number and is used as a client id
		pid_t process_id; // store the clients pid as additional useful information
		std::map<int,int> signal_fd_use_count;
		Client(int fd, pid_t pid) : socket_fd(fd), process_id(pid)  {}
		~Client() {
			if (signal_fd_use_count.size() > 0) {
				std::cerr << "not all signal fds of client " << (int)socket_fd << " were closed" << std::endl;
			}
			close(socket_fd);
		}
		void use_signal_fd(int fd) {
			int &count = signal_fd_use_count[fd];
			++count;
		}
		void release_signal_fd(int fd) {
			int &count = signal_fd_use_count[fd];
			--count;
			assert(count >= 0);
			if (count == 0) {
				close(fd);
				signal_fd_use_count.erase(fd);
			}
		}
	};
	bool operator==(const std::unique_ptr<Client> &lhs, int rhs) {
		return lhs->socket_fd == rhs;
	}

	struct ServerConnection::Impl {
		Container container_of_services;
		std::vector<std::unique_ptr<Client> > clients;
		Serializer   send;
		Deserializer received;
		Impl(ServerConnection *connection) : container_of_services(connection) {}
		~Impl() {
			std::cerr << "ServerConnection::~Impl()" << std::endl;
		}
		bool accept_client(int fd, int condition);
		bool handle_client_request(int fd, int condition);
	};

	std::vector<ServerConnection::ClientInfo> ServerConnection::get_client_info() {
		std::vector<ClientInfo> result;
		for (auto &client: d->clients) {
			result.push_back(ClientInfo());
			result.back().process_id = client->process_id;
			result.back().client_fd  = client->socket_fd;
			result.back().signal_fds = client->signal_fd_use_count;
		}
		return result;
	}

	bool ServerConnection::Impl::accept_client(int fd, int condition) {
		if (condition & POLLIN) {
			int client_socket_fd = recvfd(fd);
			std::cerr << "got (open) " << client_socket_fd << std::endl;
			if (client_socket_fd == -1) {
				std::cerr << "cannot receive socket fd" << std::endl;
			}
			// read process_id from client
			pid_t pid;
			read(client_socket_fd, &pid, sizeof(pid));
			Loop::get_default().connect(std::move(std2::make_unique<IoSource>(std::bind(&ServerConnection::Impl::handle_client_request, this, std::placeholders::_1, std::placeholders::_2), (int)client_socket_fd, (int)POLLIN)));
			// register the client
			clients.push_back(std::move(std2::make_unique<Client>(client_socket_fd, pid)));
			// send the ID back to client (the file descriptor integer number is used as ID)
			write(client_socket_fd, &client_socket_fd, sizeof(client_socket_fd));
		}
		return true;
	}

	bool ServerConnection::Impl::handle_client_request(int fd, int condition) {
		if (condition & (POLLIN|POLLHUP) ) {
			bool read_result = received.read_from(fd);
			if (!read_result) {
				std::cerr << "failed to read data from fd " << fd << std::endl;

				if (condition & POLLHUP) {
					std::cerr << "client hung up" << std::endl;

					std::cerr << "clients.size() " << clients.size() << std::endl;
					// auto client = clients.find(fd);
					auto removed_client = std::find(clients.begin(), clients.end(), fd);
					if (removed_client == clients.end()) { 
						assert(false);
					} else {
						// remove all signal fds associated with this client from all services
						for(auto &signal_fd: (*removed_client)->signal_fd_use_count) {
							std::cerr << "remove client signal fd " << signal_fd.first << std::endl;
							container_of_services.remove_signal_fd(signal_fd.second);
						}
						clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());
					}
					// tell the container that a client hung up
					// the container hast to remove all services previously owned by this client
					container_of_services.client_hung_up(fd);
					std::cerr << "clients.size() " << clients.size() << std::endl;
					return false;
				}

				return false;
			}
			unsigned saftlib_object_id;
			received.get(saftlib_object_id);
			std::cerr << "got saftlib_object_id: " << saftlib_object_id << std::endl;
			std::cerr << "found saftlib_object_id " << saftlib_object_id << std::endl;
			std::cerr << "trying to call a function" << std::endl;
			container_of_services.call_service(saftlib_object_id, fd, received, send);
			if (!send.empty()) {
				send.write_to(fd);
			}
		}
		return true;
	}


	ServerConnection::ServerConnection(const std::string &socket_name) 
		: d(std2::make_unique<Impl>(this))
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

		Loop::get_default().connect<IoSource>(std::bind(&ServerConnection::Impl::accept_client, d.get(), std::placeholders::_1, std::placeholders::_2), base_socket_fd, POLLIN);
	}

	ServerConnection::~ServerConnection() 
	{
		std::cerr << "~ServerConnection" << std::endl;
	}

	void ServerConnection::register_signal_id_for_client(int client_fd, int signal_fd)
	{
		std::cerr << "register signal fd " << signal_fd << " for client " << client_fd << std::endl;
		auto client = std::find(d->clients.begin(), d->clients.end(), client_fd);
		if (client == d->clients.end()) {
			assert(false);
		} else {
			(*client)->use_signal_fd(signal_fd);
		}
	}

	void ServerConnection::unregister_signal_id_for_client(int client_fd, int signal_fd)
	{
		std::cerr << "unregister signal fd " << signal_fd << " for client " << client_fd << std::endl;

		// auto client = d->clients.find(client_fd);
		auto client = std::find(d->clients.begin(), d->clients.end(), client_fd);
		if (client == d->clients.end()) {
			assert(false);
		} else {
			(*client)->release_signal_fd(signal_fd);
		}
	}


}