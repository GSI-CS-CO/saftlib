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

#include "client.hpp"
#include "saftbus.hpp"
#include "error.hpp"

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

namespace saftbus {

	struct ClientConnection::Impl {
		struct pollfd pfd; // file descriptor used to talk to the server
		int client_id; // the unique id of this client connection on the server
		static rtpi::mutex base_socket_mutex;
		rtpi::mutex fd_mutex;
		rtpi::mutex connection_mutex;
	};
	rtpi::mutex ClientConnection::Impl::base_socket_mutex;


	ClientConnection::ClientConnection(const std::string &socket_name) 
		: d(new Impl)
	{
		std::lock_guard<rtpi::mutex> lock1(d->base_socket_mutex);

		std::ostringstream msg;
		// msg << "ClientConnection constructor : ";
		char *socketname_env = getenv("SAFTBUS_SOCKET_PATH");
		std::string socketname = socket_name;
		if (socketname_env != nullptr) {
			socketname = socketname_env;
		}
		if (socketname.size() == 0) {
			msg << "invalid socket name (name is empty)";
			throw saftbus::Error(msg.str());
		}
		if (socketname[0] != '/') {
			msg << "saftbus socketname " << socketname << " is not an absolute pathname";
			throw saftbus::Error(msg.str());
		}
		int base_socket_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if (base_socket_fd <= 0) {
			msg << " cannot create socket: " << strerror(errno);
			throw saftbus::Error(msg.str());
		}
		struct sockaddr_un call_sockaddr_un = {.sun_family = AF_LOCAL};
		strcpy(call_sockaddr_un.sun_path, socketname.c_str());

		int connect_result = connect(base_socket_fd, (struct sockaddr *)&call_sockaddr_un , sizeof(call_sockaddr_un));
		if (connect_result != 0) {
			msg << "cannot connect to socket: " << socketname << ". Possible reasons: server not running, wrong socket path (set SAFTBUS_SOCKET_PATH evironment variable), or wrong permissions";
			throw saftbus::Error(msg.str());
		}

		int fd_pair[2];
		if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, fd_pair) != 0) {
			msg << "cannot create socket pair: " << strerror(errno);
			throw saftbus::Error(msg.str());
		}
		if (sendfd(base_socket_fd, fd_pair[0]) == -1) {
			msg << "cannot send socket pair: " << strerror(errno);
			throw saftbus::Error(msg.str());
		}
		close(fd_pair[0]);
		d->pfd.fd = fd_pair[1];

		// send the process id
		pid_t pid = getpid();
		if (write(d->pfd.fd, &pid, sizeof(pid)) != sizeof(pid)) {
			msg << "cannot read client pid" << strerror(errno) << std::endl;
			throw saftbus::Error(msg.str());
		}

		if (read(d->pfd.fd, &d->client_id, sizeof(d->client_id)) != sizeof(d->client_id)) {
			msg << "cannot read client id" << strerror(errno) << std::endl;
			throw saftbus::Error(msg.str());
		}
		// std::cerr << "got client id " << d->client_id << std::endl;
	}

	ClientConnection::~ClientConnection() 
	{
		close(d->pfd.fd);
	}


	int ClientConnection::send(Serializer &serializer, int timeout_ms)
	{
		std::lock_guard<rtpi::mutex> lock(d->fd_mutex);
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
		std::lock_guard<rtpi::mutex> lock(d->fd_mutex);
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

	int ClientConnection::atomic_send_and_receive(Serializer &serializer, Deserializer &deserializer, int timeout_ms) {
		std::lock_guard<rtpi::mutex> lock(d->connection_mutex);
		int send_result    = send(serializer, timeout_ms);
		int receive_result = receive(deserializer, timeout_ms);
		if (send_result < 0 || receive_result < 0) {
			return -1;
		}
		if (send_result != 0 && receive_result != 0) {
			return 1;
		}
		return 0;
	}

	/////////////////////////////
	/////////////////////////////
	/////////////////////////////

	struct SignalGroup::Impl {
		struct pollfd pfd;
		int fd_pair[2];
		int signal_group_id; // the integer value of the fd on the server side
		Deserializer received;
		std::vector<Proxy*> proxies;
		rtpi::mutex signal_group_mutex;
		rtpi::mutex fd_mutex;
	};

	struct Proxy::Impl {
		static std::shared_ptr<ClientConnection> connection;
		static rtpi::mutex connection_mutex;
		rtpi::mutex proxy_mutex;
		int saftbus_object_id;
		int client_id, signal_group_id; // is determined at registration time and needs to be saved for de-registration
		Serializer   send;
		Deserializer received;
		SignalGroup *signal_group;
		std::vector<std::string>   interface_names;
		std::map<std::string, int> interface_name2no_map;
	};
	std::shared_ptr<ClientConnection> Proxy::Impl::connection;
	rtpi::mutex                       Proxy::Impl::connection_mutex;

	SignalGroup::SignalGroup() 
		: d(new Impl)
	{
		// std::cerr << "SignalGroup constructor" << std::endl;
		std::ostringstream msg;
		if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, d->fd_pair) != 0) {
			msg << "cannot create socket pair: " << strerror(errno);
			throw saftbus::Error(msg.str());
		}
		// keep the other socket end in order to listen for events
		d->pfd.fd = d->fd_pair[1];
		d->pfd.events = POLLIN;
		d->signal_group_id = -1;
	}

	SignalGroup::~SignalGroup() = default;

	int SignalGroup::register_proxy(Proxy *proxy) 
	{
		std::lock_guard<rtpi::mutex> lock(d->signal_group_mutex);
		// send one of the two socket ends to the server
		// std::cerr << "register_proxy: sending one fd " << d->fd_pair[0] << " for service to send signals " << std::endl;
		// std::cerr << "register_proxy: keeping one fd " << d->fd_pair[1] << " for us to receive signals " << std::endl;

		if (d->signal_group_id == -1) {
			int fdresult = sendfd(Proxy::get_connection().d->pfd.fd, d->fd_pair[0]);
			close(d->fd_pair[0]); // close the fd after sending it to the server
			                      // if this is not closed, we will not receiver POLLHUP
			                      // when the server closed the other end (because here we
			                      // still have an open descriptor to the same end)
			if (fdresult <= 0) {
				throw saftbus::Error("SignalGroup::register_proxy cannot send file descriptor to server");
			}
		} 
		d->proxies.push_back(proxy);
		return 0;
	}

	void SignalGroup::unregister_proxy(Proxy *proxy) 
	{
		std::lock_guard<rtpi::mutex> lock(d->signal_group_mutex);
		// std::cerr << "unregister_proxy" << std::endl;
		d->proxies.erase(std::remove(d->proxies.begin(), d->proxies.end(), proxy), d->proxies.end());
	}


	int SignalGroup::get_fd()
	{
		return d->pfd.fd;
	}

	// Wait for singals to arrive. Don't wait more than timeout_ms milliseconds.
	// If there are multiple signals waiting in the queue, they are all processed before the function returns.
	// Return value:
	//   > 0 if a signal was received
	//     0 if timeout was hit
	//   < 0 in case of failure (e.g. service object was destroyed)
	int SignalGroup::wait_for_signal(int timeout_ms)
	{
		// std::cerr << "wait_for_signal(" << timeout_ms << ")" << std::endl;
		int result = wait_for_one_signal(timeout_ms);
		if (result >= 0) {
			// there was a signal, timeout was not hit. 
			// In this case look for other pending signals 
			// using a timeout of 0
			while(wait_for_one_signal(0)); // this loop will end if the timeout was hit (if no further signals are present)
		}
		return result;
	}

	// Wait for a singal signal to arrive. Don't wait more than timeout_ms milliseconds.
	// Return value:
	//   > 0 if a signal was received
	//     0 if timeout was hit
	//   < 0 in case of failure (e.g. service object was destroyed)
	int SignalGroup::wait_for_one_signal(int timeout_ms)
	{
		// std::cerr << "wait_for_one_signal(" << timeout_ms << ") on fd " << d->pfd.fd << std::endl;
		int result;
		{
			std::lock_guard<rtpi::mutex> fd_lock(d->fd_mutex);
			result = poll(&d->pfd, 1, timeout_ms);
			if (result > 0) {
				if (d->pfd.revents & (POLLIN|POLLHUP) ) {
					bool result = d->received.read_from(d->pfd.fd);
					if (!result) {
						if (d->pfd.revents & POLLHUP) {
							throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Service hung up"); 
						}
						return -1;
					} 
					int saftbus_object_id;
					int interface_no;
					int signal_no;
					d->received.get(saftbus_object_id);
					d->received.get(interface_no);
					d->received.get(signal_no);
					{
						std::lock_guard<rtpi::mutex> lock(d->signal_group_mutex);
						for (auto &proxy: d->proxies) {
							// std::cerr << "proxy object id = " << proxy->d->saftbus_object_id << "  signal_group_id = " << proxy->d->signal_group_id << std::endl;
							if (proxy->d->saftbus_object_id == saftbus_object_id) {
								d->received.save();
								proxy->signal_dispatch(interface_no, signal_no, d->received);
								d->received.restore();
							}
						}
					}
				}
				if (d->pfd.revents & POLLHUP) {
					assert(false); // did the server crash? this should never happen
				}
			}
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

	Proxy::Proxy(const std::string &object_path, SignalGroup &signal_group, const std::vector<std::string> &interface_names) 
		: d(new Impl) 
	{
		// std::cerr << "Proxy constructor for " << object_path << std::endl;
		d->signal_group = &signal_group;
		// the Proxy constructor calls the server  
		// with object_id = 1 (the Container_Service)
		unsigned container_service_object_id = 1;
		int interface_no = 0; // Container_Service has only 1 interface with interface_no 0
		int function_no = 0; // function_no 0 is register_proxy
		d->send.put(container_service_object_id);
		d->send.put(interface_no);
		d->send.put(function_no);
		d->send.put(object_path);
		d->send.put(interface_names);
		{
			d->send.put(signal_group.d->signal_group_id); 
			std::lock_guard<rtpi::mutex> lock(get_client_socket_mutex());
			int send_result = get_connection().send(d->send);
			if (send_result <= 0) {
				throw saftbus::Error("Proxy cannot send data to server");
			}
			signal_group.register_proxy(this);
			int receive_result = get_connection().receive(d->received);
			if (receive_result <= 0) {
				throw saftbus::Error("Proxy cannot receive data from server");
			}
		}
		// the response is just the object_id
		d->received.get(d->saftbus_object_id);
		d->received.get(d->client_id);
		d->received.get(d->signal_group_id);
		d->received.get(d->interface_name2no_map);
		// if we get saftbus_object_id=0, the object path was not found
		if (d->saftbus_object_id == 0) {
			std::ostringstream msg;
			msg << "object path \"" << object_path << "\" not found";
			throw saftbus::Error(msg.str());
		}
		// if we get saftbus_object_id=-1, the object path was found found bu one of the requested interfaces is not implemented
		if (d->saftbus_object_id == -1) { 
			std::ostringstream msg;
			msg << "object \"" << object_path << "\" does not implement requested interfaces: ";
			for (auto &interface_name: interface_names) {
				if (d->interface_name2no_map.find(interface_name) == d->interface_name2no_map.end()) {
					msg << "\""<< interface_name << "\"" << std::endl;
				}
			}
			throw saftbus::Error(msg.str());
		}
		if (signal_group.d->signal_group_id == -1) {
			signal_group.d->signal_group_id = d->signal_group_id;
		}
	}
	Proxy::~Proxy()
	{
		// de-register from server
		// client connection is shared among threads
		// only one thread can access the connection at a time
		std::lock_guard<rtpi::mutex> mutex_lock(d->proxy_mutex);
		d->send.put(1); // 1 is the special object id that adresses the ContainerService wich provides the unregister_proxy method
		int interface_no = 0;
		int function_no = 1; // 1 is unregister_proxy
		d->send.put(interface_no);
		d->send.put(function_no); 
		d->send.put(d->saftbus_object_id);
		d->send.put(d->client_id);
		d->send.put(d->signal_group_id);
		{
			std::lock_guard<rtpi::mutex> lock(get_client_socket_mutex());
			get_connection().send(d->send);
		}
		d->signal_group->unregister_proxy(this);
	}
	bool Proxy::signal_dispatch(int interface_no, int signal_no, Deserializer &signal_content) 
	{
		return false;
	}
	SignalGroup& Proxy::get_signal_group()
	{
		return *d->signal_group;
	}

	ClientConnection& Proxy::get_connection() {
		std::lock_guard<rtpi::mutex> lock(Proxy::Impl::connection_mutex);
		if (!Proxy::Impl::connection) {
			Proxy::Impl::connection = std::make_shared<ClientConnection>();
		}
		return *(Proxy::Impl::connection);
	}
	Serializer& Proxy::get_send()
	{
		return d->send;
	}
	Deserializer& Proxy::get_received()
	{
		return d->received;
	}

	int Proxy::get_saftbus_object_id() {
		return d->saftbus_object_id;
	}

	rtpi::mutex& Proxy::get_client_socket_mutex() {
		return get_connection().d->connection_mutex;
	}
	rtpi::mutex& Proxy::get_proxy_mutex() {
		return d->proxy_mutex;
	}

	int Proxy::interface_no_from_name(const std::string &interface_name) {
		std::map< std::string, int>::iterator it = d->interface_name2no_map.find(interface_name);
		if (it != d->interface_name2no_map.end()) {
			return it->second;
		}
		assert(0);
		return -1; // erro (maybe better throw?);
	}

	//////////////////////////////////////////	
	//////////////////////////////////////////	
	//////////////////////////////////////////	

	std::vector<std::string> Container_Proxy::gen_interface_names() {
		std::vector<std::string> result; 
		result.push_back("Container");
		return result;
	}
	Container_Proxy::Container_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group, const std::vector<std::string> &interface_names)
		: saftbus::Proxy(object_path, signal_group, interface_names)
	{
		interface_no = saftbus::Proxy::interface_no_from_name("Container");
	}
	std::shared_ptr<Container_Proxy> Container_Proxy::create(const std::string &object_path, saftbus::SignalGroup &signal_group, const std::vector<std::string> &interface_names) {
		return std::make_shared<Container_Proxy>(object_path, signal_group, gen_interface_names()); 
	}
	bool Container_Proxy::signal_dispatch(int interface_no, int signal_no, saftbus::Deserializer &signal_content) {
		if (interface_no == this->interface_no) {
		}
		return false;
	}
	bool Container_Proxy::load_plugin(const std::string & so_filename, const std::vector<std::string> &plugin_args) {
		std::lock_guard<rtpi::mutex> mutex_lock(get_proxy_mutex());
		get_send().put(get_saftbus_object_id());
		get_send().put(interface_no);
		get_send().put(2); // function_no
		get_send().put(so_filename);
		get_send().put(plugin_args);
		get_connection().atomic_send_and_receive(get_send(), get_received());
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw saftbus::Error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
		bool return_value_result_;
		get_received().get(return_value_result_);
		return return_value_result_;
	}
	bool Container_Proxy::unload_plugin(const std::string & so_filename, const std::vector<std::string> &plugin_args) {
		std::lock_guard<rtpi::mutex> mutex_lock(get_proxy_mutex());
		get_send().put(get_saftbus_object_id());
		get_send().put(interface_no);
		get_send().put(3); // function_no
		get_send().put(so_filename);
		get_send().put(plugin_args);
		get_connection().atomic_send_and_receive(get_send(), get_received());
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw saftbus::Error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
		bool return_value_result_;
		get_received().get(return_value_result_);
		return return_value_result_;
	}
	bool Container_Proxy::remove_object(const std::string & object_path	) {
		std::lock_guard<rtpi::mutex> mutex_lock(get_proxy_mutex());
		get_send().put(get_saftbus_object_id());
		get_send().put(interface_no);
		get_send().put(4); // function_no
		get_send().put(object_path);
		get_connection().atomic_send_and_receive(get_send(), get_received());
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw saftbus::Error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
		bool return_value_result_;
		get_received().get(return_value_result_);
		return return_value_result_;
	}
	void Container_Proxy::quit(	) {
		std::lock_guard<rtpi::mutex> mutex_lock(get_proxy_mutex());
		get_send().put(get_saftbus_object_id());
		get_send().put(interface_no);
		get_send().put(5); // function_no
		get_connection().atomic_send_and_receive(get_send(), get_received());
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw saftbus::Error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
	}
	SaftbusInfo Container_Proxy::get_status(	) {
		std::lock_guard<rtpi::mutex> mutex_lock(get_proxy_mutex());
		get_send().put(get_saftbus_object_id());
		get_send().put(interface_no);
		get_send().put(6); // function_no
		get_connection().atomic_send_and_receive(get_send(), get_received());
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw saftbus::Error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
		SaftbusInfo return_value_result_;
		get_received().get(return_value_result_);
		return return_value_result_;
	}
}