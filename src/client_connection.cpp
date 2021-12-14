#include "client_connection.hpp"

#include <server_connection.hpp>
#include <saftbus.hpp>

#include <sstream>
#include <iostream>
#include <mutex>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>

namespace mini_saftlib {

	struct ClientConnection::Impl {
		struct pollfd pfd; // file descriptor used to talk to the server
		int client_id; // the unique id of this client connection on the server
		std::mutex m_base_socket;
		std::mutex m_client_socket;
	};


	ClientConnection::ClientConnection(const std::string &socket_name) 
		: d(std::make_unique<Impl>())
	{
		std::lock_guard<std::mutex> lock1(d->m_base_socket);

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
		d->pfd.fd = fd_pair[1];

		if (read(d->pfd.fd, &d->client_id, sizeof(d->client_id)) != sizeof(d->client_id)) {
			msg << "cannot read client id" << strerror(errno) << std::endl;
			throw std::runtime_error(msg.str());
		}
		std::cerr << "got client id " << d->client_id << std::endl;
	}

	void ClientConnection::send_call()
	{
		// std::lock_guard<std::mutex> lock(d->m_client_socket);
		// std::vector<int> data(1000,0);
		// SerDes serdes;
		// serdes.put(ServerConnection::CALL);
		// serdes.put(data);
		// serdes.write_to(d->pfd.fd);
	}



	ClientConnection::~ClientConnection() 
	{
		close(d->pfd.fd);
	}


	int ClientConnection::send(Serializer &serdes, int timeout_ms)
	{
		std::lock_guard<std::mutex> lock(d->m_client_socket);
		d->pfd.events = POLLOUT;
		int result;
		if ((result = poll(&d->pfd, 1, timeout_ms)) > 0) {
			if (d->pfd.revents & POLLOUT) {
				serdes.write_to(d->pfd.fd);
			}
		}
		return result; // 0 in case of timeout
	}
	int ClientConnection::receive(Deserializer &serdes, int timeout_ms)
	{
		std::lock_guard<std::mutex> lock(d->m_client_socket);
		int result;
		d->pfd.events = POLLIN;
		if ((result = poll(&d->pfd, 1, timeout_ms)) > 0) {
			if (d->pfd.revents & POLLIN) {
				serdes.read_from(d->pfd.fd);
			}
		}
		return result;
	}

	/////////////////////////////
	/////////////////////////////
	/////////////////////////////

	struct SignalGroup::Impl {
		struct pollfd pfd;
		int fd_pair[2];
		Deserializer received;
		std::vector<std::shared_ptr<Proxy> > proxies;
		std::mutex m1, m2;
	};

	struct Proxy::Impl {
		int saftlib_object_id;
		Serializer   send;
		Deserializer received;
		static std::shared_ptr<ClientConnection> connection;
	};
	std::shared_ptr<ClientConnection> Proxy::Impl::connection;

	SignalGroup::SignalGroup() 
		: d(std::make_unique<Impl>())
	{
		std::ostringstream msg;
		if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, d->fd_pair) != 0) {
			msg << "cannot create socket pair: " << strerror(errno);
			throw std::runtime_error(msg.str());
		}
		// close(d->fd_pair[0]);
		// keep the other socket end in order to listen for events
		d->pfd.fd = d->fd_pair[1];
		d->pfd.events = POLLIN | POLLHUP | POLLERR;
	}

	SignalGroup::~SignalGroup() = default;

	int SignalGroup::send_fd(Proxy &proxy) 
	{
		// send one of the two socket ends to the server
		std::cerr << "sending socket pair for signals " << std::endl;
		return sendfd(Proxy::connection()->d->pfd.fd, d->fd_pair[0]);
	}


	int SignalGroup::wait_for_signal(int timeout_ms)
	{
		int result = wait_for_one_signal(timeout_ms);
		if (result) {
			// there was a signal, timeout was not hit. 
			// In this case look for other pending signals 
			// using a timeout of 0
			while(wait_for_one_signal(0)); // this loop will end if the timeout was hit (if no further signals are present)
		}
		return result;
	}

	int SignalGroup::wait_for_one_signal(int timeout_ms)
	{
		int result;
		{
			std::lock_guard<std::mutex> lock1(d->m1);
			if ((result = poll(&d->pfd, 1, timeout_ms)) > 0) {
				if (d->pfd.revents & POLLIN) {
					bool result = d->received.read_from(d->pfd.fd);
					if (!result) {
						std::cerr << "failed to read data from fd " << d->pfd.fd << std::endl;
						return -1;
					} 
					int saftlib_object_id;
					int interface;
					d->received.get(saftlib_object_id);
					d->received.get(interface);
					for (auto &proxy: d->proxies) {
						if (proxy->d->saftlib_object_id == saftlib_object_id) {
							proxy->signal_dispatch(interface, d->received);
						}
					}
				}
				if (d->pfd.revents & POLLHUP) {
					std::cerr << __FILE__ << ": " << __LINE__ << ": did server crash? this should not happen" << std::endl;
				}
			}
		}
		{
			std::lock_guard<std::mutex> lock2(d->m2);
		}
		return result;
	}

	SignalGroup globalSignalGroup;


	/////////////////////////////
	/////////////////////////////
	/////////////////////////////

	std::shared_ptr<ClientConnection> Proxy::connection() {
		if (!static_cast<bool>(Proxy::Impl::connection)) {
			Proxy::Impl::connection = std::make_shared<ClientConnection>();
		}
		return Proxy::Impl::connection;
	}



	Proxy::Proxy(const std::string &object_path, SignalGroup &signal_group) 
		: d(std::make_unique<Impl>()) 
		{
			// the Proxy constructor calls the server for 
			// the saftlib_object_id that was given to this 
			// particular object_path;
			// int &fd = d->get_connection()->d->pfd.fd;
			// std::mutex &m = d->get_connection()->d->m_client_socket;
			d->send.put(ServerConnection::GET_SAFTLIB_OBJECT_ID);
			d->send.put(object_path);
			{
				// client connection is shared amongh threads
				// only one thread cann access the connection at a time
				std::lock_guard<std::mutex> lock(connection()->d->m_client_socket);
				connection()->send(d->send);
				signal_group.send_fd(*this);
				connection()->receive(d->received);
			}
			d->received.get(d->saftlib_object_id);
			std::cerr << "Proxy got saftlib_object_id: " << d->saftlib_object_id << std::endl;
		}
	Proxy::~Proxy() = default;

}