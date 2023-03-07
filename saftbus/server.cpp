/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#include "server.hpp"
#include "service.hpp"
#include "saftbus.hpp"
#include "loop.hpp"

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
		SourceHandle io_source; // use this to disconnect the source in the destructor
		std::map<int,int> signal_fd_use_count;
		Client(int fd, pid_t pid, SourceHandle h) : socket_fd(fd), process_id(pid), io_source(h)
		{}
		~Client() {
			Loop::get_default().remove(io_source);
			if (signal_fd_use_count.size() > 0) {
				//===std::cerr << "not all signal fds of client " << (int)socket_fd << " were closed" << std::endl;
			}
			// close socket_fd
			close(socket_fd);
			// close all signal_fds
			for (auto& fd_use_count: signal_fd_use_count) {
				// std::cout << "close signal fd " << fd_use_count.first << " with " << fd_use_count.second << " users " << std::endl;
				close(fd_use_count.first);
			}
		}
		void use_signal_fd(int fd) {
			int &count = signal_fd_use_count[fd];
			++count;
			// std::cout << "Client::use_signal_fd(" << fd << ") count=" << count << std::endl;
		}
		void release_signal_fd(int fd) {
			int &count = signal_fd_use_count[fd];
			--count;
			if (count < 0) {
				std::cerr << "fatal error: " << __FILE__ << ":" << __LINE__ << " signal use count < 0 must not happen" << std::endl;
				assert(false);
			}
			// don't close even if use count reaches 0, because the client might create other proxies later
			// if fd would be closed here, client must be able to detect this in the proxy constructor and 
			// send a new socket-pair-fd so that the connection can be re-established
			// All signal file descriptors are closed when the client disconnects
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
		int calling_client_id; // this is equal to the client id as long as a client request is handled
		Impl(ServerConnection *connection) : container_of_services(connection), calling_client_id(-1) {}
		~Impl() {
			//===std::cerr << "ServerConnection::~Impl()" << std::endl;
		}
		bool accept_client(int fd, int condition);
		bool handle_client_request(int fd, int condition);
		void client_hung_up(int client_fd);
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
			// std::cout << "got (open) " << client_socket_fd << std::endl;
			if (client_socket_fd == -1) {
				std::cerr << "cannot receive socket fd" << std::endl;
				assert(false);
			}
			// read process_id from client
			pid_t pid;
			int result = read(client_socket_fd, &pid, sizeof(pid));
			if (result != sizeof(pid)) {
				std::cerr << "Error in ServerConnection::Impl::accept_client: read unexpected number of bytes" << std::endl;
			}
			auto handle = Loop::get_default().connect<IoSource>(std::bind(&ServerConnection::Impl::handle_client_request, this, std::placeholders::_1, std::placeholders::_2), client_socket_fd, POLLIN | POLLHUP | POLLERR);
			// register the client
			clients.push_back(std::move(std::unique_ptr<Client>(new Client(client_socket_fd, pid, handle))));
			// send the ID back to client (the file descriptor integer number is used as ID)
			result = write(client_socket_fd, &client_socket_fd, sizeof(client_socket_fd));
			if (result != sizeof(client_socket_fd)) {
				std::cerr << "Error in ServerConnection::Impl::accept_client: write returns unexpected number of bytes" << std::endl;
			}
		}
		return true;
	}

	bool ServerConnection::Impl::handle_client_request(int fd, int condition) {
		// The calling_client_id should be reset to -1 at end of scope.
		// This anonymous struct guarantees that this is the case
		struct ClientID {
			int &ref;
			ClientID(int &id_ref, int id_value) : ref(id_ref) { id_ref = id_value; }
			~ClientID() { ref = -1; }
		} ccid(calling_client_id, fd); 

		// if (condition & POLLIN)  { std::cout << "POLLIN" << std::endl; }
		// if (condition & POLLHUP) { std::cout << "POLLHUP" << std::endl; }
		// if (condition & POLLERR) { std::cout << "POLLERR" << std::endl; }

		if (condition & (POLLIN|POLLHUP) ) {
			// if POLLHUP is received, there may still be data inside the pipe
			// we have to loop and read until all data is read and all remaining actions 
			// are executed. But not more then 100 times. 
			bool read_result = received.read_from(fd);
			if (!read_result) {
				// //===std::cerr << "failed to read data from fd " << fd << std::endl;
				client_hung_up(fd);
				// std::cout << "remove client IoSource " << fd << std::endl;
				return false;
			}
			unsigned saftbus_object_id;
			received.get(saftbus_object_id);
			// //===std::cerr << "got saftbus_object_id: " << saftbus_object_id << std::endl;
			// //===std::cerr << "found saftbus_object_id " << saftbus_object_id << std::endl;
			// //===std::cerr << "trying to call a function" << std::endl;
			if (!container_of_services.call_service(saftbus_object_id, fd, received, send)) { 
				// call_service returns false if the service object was not found
				// in this case an exception is sent to the Proxy 
				send.put(saftbus::FunctionResult::EXCEPTION);
				std::string what("remote call failed because service object was not found");
				send.put(what);
			} 
			if (!send.empty()) {
				send.write_to(fd);
			}
		}
		return true;
	}

	void ServerConnection::Impl::client_hung_up(int client_fd) 
	{
		// //===std::cerr << "clients.size() " << clients.size() << std::endl;
		auto removed_client = std::find(clients.begin(), clients.end(), client_fd);
		if (removed_client == clients.end()) { 
			calling_client_id = -1;
			assert(false);
		} else {
			// remove all signal fds associated with this client from all services
			for(auto &sigfd_usecount: (*removed_client)->signal_fd_use_count) {
				int sigfd = sigfd_usecount.first;
				// int count = sigfd_usecount.second;
				// //===std::cerr << "remove client signal fd " << sigfd << std::endl;
				container_of_services.remove_signal_fd(sigfd);
			}
			clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
		}
		// tell the container that a client hung up
		// the container hast to remove all services previously owned by this client
		container_of_services.client_hung_up(client_fd);
		// //===std::cerr << "clients.size() " << clients.size() << std::endl;
	}


	ServerConnection::ServerConnection(const std::vector<std::pair<std::string, std::vector<std::string> > > &plugins_and_args, const std::string &socket_name) 
		: d(new Impl(this))
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
		if (mkdir(dirname.c_str(), 0755)) {
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
		chmod(dirname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | 
			                   S_IRGRP |           S_IXGRP | 
			                   S_IROTH |           S_IXOTH );

		Loop::get_default().connect<IoSource>(std::bind(&ServerConnection::Impl::accept_client, d.get(), std::placeholders::_1, std::placeholders::_2), base_socket_fd, POLLIN | POLLHUP | POLLERR);

		for (auto &plugin_and_args: plugins_and_args) {
			auto& plugin_name = plugin_and_args.first;
			auto& plugin_args = plugin_and_args.second;
			if (!d->container_of_services.load_plugin(plugin_name, plugin_args)) {
				std::string msg("cannot load plugin ");
				msg.append(plugin_name);
				throw std::runtime_error(msg);
			}
		}
	}

	ServerConnection::~ServerConnection() 
	{
		//===std::cerr << "~ServerConnection" << std::endl;
	}

	void ServerConnection::register_signal_id_for_client(int client_fd, int signal_fd)
	{
		//===std::cerr << "register signal fd " << signal_fd << " for client " << client_fd << std::endl;
		auto client = std::find(d->clients.begin(), d->clients.end(), client_fd);
		if (client == d->clients.end()) {
			assert(false);
		} else {
			(*client)->use_signal_fd(signal_fd);
		}
	}

	void ServerConnection::unregister_signal_id_for_client(int client_fd, int signal_fd)
	{
		//===std::cerr << "unregister signal fd " << signal_fd << " for client " << client_fd << std::endl;

		// auto client = d->clients.find(client_fd);
		auto client = std::find(d->clients.begin(), d->clients.end(), client_fd);
		if (client == d->clients.end()) {
			assert(false);
		} else {
			(*client)->release_signal_fd(signal_fd);
		}
	}

	int ServerConnection::get_calling_client_id() {
		return d->calling_client_id;
	}
	void set_owner();
	void release_owner();
	void owner_only();


	Container* ServerConnection::get_container() {
		return &(d->container_of_services);
	}


}