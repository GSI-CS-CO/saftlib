#include "client.hpp"
#include "make_unique.hpp"
#include "saftbus.hpp"

#include <sstream>
#include <iostream>
#include <mutex>
#include <cassert>

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
		: d(std2::make_unique<Impl>())
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

	ClientConnection::~ClientConnection() 
	{
		close(d->pfd.fd);
	}


	int ClientConnection::send(Serializer &serializer, int timeout_ms)
	{
		std::lock_guard<std::mutex> lock(d->m_client_socket);
		d->pfd.events = POLLOUT | POLLHUP;
		int result;
		if ((result = poll(&d->pfd, 1, timeout_ms)) > 0) {
			if (d->pfd.revents & POLLOUT) {
				serializer.write_to(d->pfd.fd);
			}
			if (d->pfd.revents & POLLHUP) {
				return -1;
			}
		}
		return result; // 0 in case of timeout
	}
	int ClientConnection::receive(Deserializer &deserializer, int timeout_ms)
	{
		std::lock_guard<std::mutex> lock(d->m_client_socket);
		int result;
		d->pfd.events = POLLIN | POLLHUP;
		if ((result = poll(&d->pfd, 1, timeout_ms)) > 0) {
			if (d->pfd.revents & POLLIN) {
				deserializer.read_from(d->pfd.fd);
			}
			if (d->pfd.revents & POLLHUP) {
				return -1;
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
		std::vector<Proxy*> proxies;
		std::mutex m1, m2;
	};

	struct Proxy::Impl {
		int saftlib_object_id;
		int client_id, signal_group_id; // is determined at registration time and needs to be saved for de-registration
		Serializer   send;
		Deserializer received;
		SignalGroup *signal_group;
	};

	SignalGroup::SignalGroup() 
		: d(std2::make_unique<Impl>())
	{
		std::ostringstream msg;
		if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, d->fd_pair) != 0) {
			msg << "cannot create socket pair: " << strerror(errno);
			throw std::runtime_error(msg.str());
		}
		// close(d->fd_pair[0]);
		// close(d->fd_pair[1]);
		// keep the other socket end in order to listen for events
		d->pfd.fd = d->fd_pair[1];
		d->pfd.events = POLLIN | POLLHUP | POLLERR;
	}

	SignalGroup::~SignalGroup() = default;

	int SignalGroup::send_fd(Proxy &proxy) 
	{
		// send one of the two socket ends to the server
		std::cerr << "sending socket pair for signals " << std::endl;
		return sendfd(Proxy::get_connection().d->pfd.fd, d->fd_pair[0]);
		return 0;
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
					assert(false); // did the server crash? this should never happen
				}
			}
		}
		{
			std::lock_guard<std::mutex> lock2(d->m2);
		}
		return result;
	}
	SignalGroup& SignalGroup::get_global()
	{
		static SignalGroup signal_group;
		return signal_group;
	}


	/////////////////////////////
	/////////////////////////////
	/////////////////////////////

	Proxy::Proxy(const std::string &object_path, SignalGroup &signal_group) 
		: d(std2::make_unique<Impl>()) 
		{
			d->signal_group = &signal_group;
			// the Proxy constructor calls the server for 
			// with object_id = 1 (the CoreService)
			unsigned container_service_object_id = 1;
			int interface_no = 0;
			int function_no = 0; // 0 is register_proxy
			d->send.put(container_service_object_id);
			d->send.put(interface_no);
			d->send.put(function_no);
			d->send.put(object_path);
			{
				std::lock_guard<std::mutex> lock(get_connection().d->m_client_socket);
				int send_result    = get_connection().send(d->send);
				if (send_result <= 0) {
					throw std::runtime_error("Proxy cannot send data to server");
				}
				signal_group.send_fd(*this);
				int receive_result = get_connection().receive(d->received);
				if (receive_result <= 0) {
					throw std::runtime_error("Proxy cannot receive data from server");
				}
			}
			// the response is just the object_id
			d->received.get(d->saftlib_object_id);
			d->received.get(d->client_id);
			d->received.get(d->signal_group_id);
			// if we get saftlib_object_id=0, the object path was not found
			if (!d->saftlib_object_id) {
				std::ostringstream msg;
				msg << "object path \"" << object_path << "\" not found" << std::endl;
				throw std::runtime_error(msg.str());
			}
			std::cerr << "Proxy got saftlib_object_id: " << d->saftlib_object_id << std::endl;
		}
	Proxy::~Proxy()
	{
		std::cerr << "destroy Proxy" << std::endl;
		// de-register from signal_group
		// d->signal_group->d->proxies.erase(std::remove(d->signal_group->d->proxies.begin(), d->signal_group->d->proxies.end(), this),
		// 	                              d->signal_group->d->proxies.end());
		// de-register from server
		// client connection is shared among threads
		// only one thread can access the connection at a time
		d->send.put(d->saftlib_object_id);
		int interface_no = 0;
		int function_no = 1; // 1 is unregister_proxy
		d->send.put(interface_no);
		d->send.put(function_no); 
		d->send.put(d->saftlib_object_id);
		d->send.put(d->client_id);
		d->send.put(d->signal_group_id);
		{
			std::lock_guard<std::mutex> lock(get_connection().d->m_client_socket);
			int send_result    = get_connection().send(d->send);
			if (send_result <= 0) {
				return; // if we cannot communicate with server, there is no point doing anything more
			}
			int receive_result = get_connection().receive(d->received);
			if (receive_result <= 0) {
				return; // if we cannot communicate with server, there is no point doing anything more
			}
		}
		bool result;
		std::cerr << "waiting for response from de-register" << std::endl;
		d->received.get(result);
		assert(result); // de-registration should always succeed
		std::cerr << "Proxy de-registration successful" << std::endl;
	}


	ClientConnection& Proxy::get_connection() {
		static ClientConnection connection;
		return connection;
	}
	Serializer& Proxy::get_send()
	{
		return d->send;
	}
	Deserializer& Proxy::get_received()
	{
		return d->received;
	}

	int Proxy::get_saftlib_object_id() {
		return d->saftlib_object_id;
	}

	std::mutex& Proxy::get_client_socket() {
		return get_connection().d->m_client_socket;
	}

	//////////////////////////////////////////	
	//////////////////////////////////////////	
	//////////////////////////////////////////	

	ContainerService_Proxy::ContainerService_Proxy(const std::string &object_path, SignalGroup &signal_group)
		: Proxy(object_path, signal_group)
	{}

	std::shared_ptr<ContainerService_Proxy> ContainerService_Proxy::create(SignalGroup &signal_group)
	{
		return std::make_shared<ContainerService_Proxy>("/de/gsi/saftlib", signal_group);
	}
	bool ContainerService_Proxy::signal_dispatch(int interface, Deserializer &signal_content)
	{
		std::cerr << "ContainerService_Proxy received a signal from interface " << interface << std::endl;
		return true;
	}

	void ContainerService_Proxy::quit() 
	{
		get_send().put(get_saftlib_object_id());
		unsigned interface_no, function_no;
		get_send().put(interface_no = 0);
		get_send().put(function_no  = 2); 
		std::lock_guard<std::mutex> lock(get_client_socket());
		get_connection().send(get_send());
	}
}