#include "client.hpp"
#include "make_unique.hpp"
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

		// send the process id
		pid_t pid = getpid();
		if (write(d->pfd.fd, &pid, sizeof(pid)) != sizeof(pid)) {
			msg << "cannot read client pid" << strerror(errno) << std::endl;
			throw std::runtime_error(msg.str());
		}

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
		int signal_group_id; // the integer value of the fd on the server side
		Deserializer received;
		std::vector<Proxy*> proxies;
		std::mutex m1, m2;
	};

	struct Proxy::Impl {
		static std::shared_ptr<ClientConnection> connection;
		int saftlib_object_id;
		int client_id, signal_group_id; // is determined at registration time and needs to be saved for de-registration
		Serializer   send;
		Deserializer received;
		SignalGroup *signal_group;
		std::vector<std::string>   interface_names;
		std::map<std::string, int> interface_name2no_map;
	};
	std::shared_ptr<ClientConnection> Proxy::Impl::connection;

	SignalGroup::SignalGroup() 
		: d(std2::make_unique<Impl>())
	{
		std::cerr << "SignalGroup constructor" << std::endl;
		std::ostringstream msg;
		if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, d->fd_pair) != 0) {
			msg << "cannot create socket pair: " << strerror(errno);
			throw std::runtime_error(msg.str());
		}
		// keep the other socket end in order to listen for events
		d->pfd.fd = d->fd_pair[1];
		d->pfd.events = POLLIN;
		d->signal_group_id = -1;
	}

	SignalGroup::~SignalGroup() = default;

	int SignalGroup::register_proxy(Proxy *proxy) 
	{
		// send one of the two socket ends to the server
		std::cerr << "register_proxy: sending one fd " << d->fd_pair[0] << " for signals " << std::endl;

		if (d->signal_group_id == -1) {
			int fdresult = sendfd(Proxy::get_connection().d->pfd.fd, d->fd_pair[0]);
			close(d->fd_pair[0]); // close the fd after sending it to the server
			                      // if this is not closed, we will not receiver POLLHUP
			                      // when the server closed the other end (because here we
			                      // still have an open descriptor to the same end)
			if (fdresult <= 0) {
				std::cerr << "SignalGroup::register_proxy cannot send file descriptor to server" << std::endl;
			}
		} 
		d->proxies.push_back(proxy);
		return 0;
	}

	void SignalGroup::unregister_proxy(Proxy *proxy) 
	{
		std::cerr << "unregister_proxy" << std::endl;
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
		std::cerr << "wait_for_signal(" << timeout_ms << ")" << std::endl;
		int result = wait_for_one_signal(timeout_ms);
		if (result >= 0) {
			std::cerr << "." << std::endl;
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
		std::cerr << "wait_for_one_signal(" << timeout_ms << ")" << std::endl;
		int result;
		{
			std::cerr << "wait for mutex" << std::endl;
			std::lock_guard<std::mutex> lock1(d->m1);
			std::cerr << "SignalGroup poll call " << d->pfd.events << " " << timeout_ms << std::endl;
			result = poll(&d->pfd, 1, timeout_ms);
			std::cerr << "SignalGroup poll done " << result << std::endl;
			if (result > 0) {

				if (d->pfd.revents & (POLLIN|POLLHUP) ) {
					if (d->pfd.revents & POLLIN)  std::cerr << "POLLIN"  << std::endl;
					if (d->pfd.revents & POLLHUP) std::cerr << "POLLHUP" << std::endl;
					// std::cerr << "POLLIN|POLLHUP" << std::endl;
					bool result = d->received.read_from(d->pfd.fd);
					if (!result) {
						std::cerr << "failed to read data from fd " << d->pfd.fd << std::endl;
						if (d->pfd.revents & POLLHUP) {
							std::cerr << "service hung up" << std::endl;
							throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Service hung up"); 
						}
						return -1;
					} 
					int saftlib_object_id;
					int interface_no;
					int signal_no;
					d->received.get(saftlib_object_id);
					d->received.get(interface_no);
					d->received.get(signal_no);
					std::cerr << "object_id = " << saftlib_object_id << " inteface = " << interface_no << "   signal = " << signal_no << " d->proxies.size()=" << d->proxies.size() <<  std::endl;
					for (auto &proxy: d->proxies) {
						std::cerr << "proxy object id = " << proxy->d->saftlib_object_id << "  signal_group_id = " << proxy->d->signal_group_id << std::endl;
						if (proxy->d->saftlib_object_id == saftlib_object_id) {
							d->received.save();
							proxy->signal_dispatch(interface_no, signal_no, d->received);
							d->received.restore();
						}
					}
				}
				if (d->pfd.revents & POLLHUP) {
					assert(false); // did the server crash? this should never happen
				}
			}
			std::cerr << "SignalGroup poll call done" << std::endl;
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

	Proxy::Proxy(const std::string &object_path, SignalGroup &signal_group, const std::vector<std::string> &interface_names) 
		: d(std2::make_unique<Impl>()) 
	{
		std::cerr << "Proxy constructor for " << object_path << std::endl;
		d->signal_group = &signal_group;
		std::lock_guard<std::mutex> lock2(d->signal_group->d->m2);
		std::lock_guard<std::mutex> lock1(d->signal_group->d->m1);
		// the Proxy constructor calls the server for 
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
			std::lock_guard<std::mutex> lock(get_connection().d->m_client_socket);
			d->send.put(signal_group.d->signal_group_id); 
			int send_result    = get_connection().send(d->send);
			if (send_result <= 0) {

				throw std::runtime_error("Proxy cannot send data to server");
			}
			signal_group.register_proxy(this);
			int receive_result = get_connection().receive(d->received);
			if (receive_result <= 0) {
				throw std::runtime_error("Proxy cannot receive data from server");
			}
		}
		// the response is just the object_id
		d->received.get(d->saftlib_object_id);
		d->received.get(d->client_id);
		d->received.get(d->signal_group_id);
		d->received.get(d->interface_name2no_map);
		// if we get saftlib_object_id=0, the object path was not found
		if (d->saftlib_object_id == 0) {
			std::ostringstream msg;
			msg << "object path \"" << object_path << "\" not found" << std::endl;
			throw std::runtime_error(msg.str());
		}
		// if we get saftlib_object_id=-1, the object path was found found bu one of the requested interfaces is not implemented
		if (d->saftlib_object_id == -1) { 
			std::ostringstream msg;
			msg << "object \"" << object_path << "\" does not implement requested interfaces: ";
			for (auto &interface_name: interface_names) {
				if (d->interface_name2no_map.find(interface_name) == d->interface_name2no_map.end()) {
					msg << "\""<< interface_name << "\"" << std::endl;
				}
			}
			throw std::runtime_error(msg.str());
		}
		if (signal_group.d->signal_group_id == -1) {
			std::cerr << "set signal_group_id " << d->signal_group_id << std::endl;
			signal_group.d->signal_group_id = d->signal_group_id;
		}
		std::cerr << "Proxy got saftlib_object_id: " << d->saftlib_object_id << std::endl;
		std::cerr << "interface name to no mapping: " << std::endl;
		for (auto &pair: d->interface_name2no_map) {
			std::cerr << "\t" << pair.first << " -> " << pair.second << std::endl;
		}
	}
	Proxy::~Proxy()
	{
		std::lock_guard<std::mutex> lock2(d->signal_group->d->m2);
		std::lock_guard<std::mutex> lock1(d->signal_group->d->m1);
		std::cerr << "destroy Proxy" << std::endl;
		// de-register from server
		// client connection is shared among threads
		// only one thread can access the connection at a time
		d->send.put(1); // 1 is the special object id that adresses the ContainerService wich provides the unregister_proxy method
		int interface_no = 0;
		int function_no = 1; // 1 is unregister_proxy
		d->send.put(interface_no);
		d->send.put(function_no); 
		d->send.put(d->saftlib_object_id);
		d->send.put(d->client_id);
		d->send.put(d->signal_group_id);
		{
			std::lock_guard<std::mutex> lock(get_connection().d->m_client_socket);
			get_connection().send(d->send);
		}
		d->signal_group->unregister_proxy(this);
	}
	bool Proxy::signal_dispatch(int interface_no, int signal_no, Deserializer &signal_content) 
	{
		return false;
	}

	ClientConnection& Proxy::get_connection() {
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

	int Proxy::get_saftlib_object_id() {
		return d->saftlib_object_id;
	}

	std::mutex& Proxy::get_client_socket() {
		return get_connection().d->m_client_socket;
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
		return std2::make_unique<Container_Proxy>(object_path, signal_group, gen_interface_names()); 
	}
	bool Container_Proxy::signal_dispatch(int interface_no, int signal_no, saftbus::Deserializer &signal_content) {
		if (interface_no == this->interface_no) {
		}
		return false;
	}
	bool Container_Proxy::load_plugin(const std::string & so_filename	) {
		get_send().put(get_saftlib_object_id());
		get_send().put(interface_no);
		get_send().put(2); // function_no
		get_send().put(so_filename);
		{
			std::lock_guard<std::mutex> lock(get_client_socket());
			get_connection().send(get_send());
			get_connection().receive(get_received());
		}
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw std::runtime_error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
		bool return_value_result_;
		get_received().get(return_value_result_);
		return return_value_result_;
	}
	bool Container_Proxy::remove_object(const std::string & object_path	) {
		get_send().put(get_saftlib_object_id());
		get_send().put(interface_no);
		get_send().put(3); // function_no
		get_send().put(object_path);
		{
			std::lock_guard<std::mutex> lock(get_client_socket());
			get_connection().send(get_send());
			get_connection().receive(get_received());
		}
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw std::runtime_error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
		bool return_value_result_;
		get_received().get(return_value_result_);
		return return_value_result_;
	}
	void Container_Proxy::quit(	) {
		get_send().put(get_saftlib_object_id());
		get_send().put(interface_no);
		get_send().put(4); // function_no
		{
			std::lock_guard<std::mutex> lock(get_client_socket());
			get_connection().send(get_send());
			get_connection().receive(get_received());
		}
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw std::runtime_error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
	}



	// this is hand-written
	SaftbusInfo Container_Proxy::get_status()
	{
		SaftbusInfo result;
		get_send().put(get_saftlib_object_id());
		unsigned interface_no, function_no;
		get_send().put(interface_no = 0);
		get_send().put(function_no  = 5);
		{
			std::lock_guard<std::mutex> lock(get_client_socket());
			get_connection().send(get_send());
			get_connection().receive(get_received());
		}
		saftbus::FunctionResult function_result_;
		get_received().get(function_result_);
		if (function_result_ == saftbus::FunctionResult::EXCEPTION) {
			std::string what;
			get_received().get(what);
			throw std::runtime_error(what);
		}
		assert(function_result_ == saftbus::FunctionResult::RETURN);
		size_t object_count;
		get_received().get(object_count);
		for (int i = 0; i < 5; ++i) std::cerr << std::endl;
		for (unsigned i = 0; i < object_count; ++i) {
			result.object_infos.push_back(SaftbusInfo::ObjectInfo());
			auto &object_info = result.object_infos.back();

			get_received().get(object_info.object_id);
			get_received().get(object_info.object_path);
			get_received().get(object_info.interface_names);
			get_received().get(object_info.signal_fds_use_count);
			get_received().get(object_info.owner);

			std::cerr << object_info.object_path << "=>" << object_info.object_id << " owner:" << object_info.owner << " sig_fd/use_cnt: ";
			for (auto &signal_fd_use_count: object_info.signal_fds_use_count) {
				std::cerr << signal_fd_use_count.first << "/" << signal_fd_use_count.second << " ";
			}
			std::cerr << std::endl;
			for (auto &interface_name: object_info.interface_names) {
				std::cerr << "    " << interface_name << std::endl;
			}
			std::cerr << std::endl;
		}
		std::cerr << std::endl;
		size_t client_count;
		get_received().get(client_count);
		for (unsigned i = 0; i < client_count; ++i) {
			result.client_infos.push_back(SaftbusInfo::ClientInfo());
			auto &client_info = result.client_infos.back();
			get_received().get(client_info.process_id);
			get_received().get(client_info.client_fd);
			get_received().get(client_info.signal_fds);
			std::cerr << "client " << client_info.client_fd << "(pid=" << client_info.process_id << ") with signal_fds: ";
			for (auto &signal_fd: client_info.signal_fds) {
				std::cerr << signal_fd.first << "/" << signal_fd.second << " " ;
			}
			std::cerr << std::endl;
		}
		for (int i = 0; i < 5; ++i) std::cerr << std::endl;
		return result;
	}
}