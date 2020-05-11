#include "ProxyConnection.h"
#include "Proxy.h"
#include "saftbus.h"
#include "core.h"

#include <iostream>
#include <sstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <errno.h>
#include <algorithm>
#include <ctime>
#include <cstdlib>

#include <unistd.h>
//#include <giomm/dbuserror.h>

namespace saftbus
{

ProxyConnection::ProxyConnection(const std::string &base_name)
{
	std::unique_lock<std::mutex> lock(_socket_mutex);

	char *socketname_env = getenv(saftbus_socket_environment_variable_name);
	std::string socketname = base_name;
	if (socketname_env != nullptr) {
		socketname = socketname_env;
	}
	if (socketname[0] != '/') {
		throw std::runtime_error("saftbus socket is not an absolute pathname");
	}

	int server_socket = socket(PF_LOCAL, SOCK_DGRAM, 0);
	if (server_socket <= 0) {
		throw std::runtime_error("ProxyConnection::ProxyConnection() : cannot create socket");
	}
	_address.sun_family = AF_LOCAL;

	strcpy(_address.sun_path, socketname.c_str());
	int connect_result = connect( server_socket, (struct sockaddr *)&_address , sizeof(_address));
	if (connect_result != 0) {
		throw std::runtime_error("Cannot connect to socket. Possible reasons: all sockets busy, saftd not running, or wrong socket permissions");
	}

	int fd_pair[2];
	if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, fd_pair) != 0) {
		throw std::runtime_error("Cannot create socket pair");
	}
	if (sendfd(server_socket, fd_pair[0]) == -1) {
		throw std::runtime_error("couldn't send socket pair");
	}
	close(fd_pair[0]);
	_create_socket = fd_pair[1];

	saftbus::write(get_fd(), saftbus::SENDER_ID);  // ask the saftd for an ID on the saftbus
	saftbus::read(get_fd(), _saftbus_id);  
}

int ProxyConnection::get_saftbus_index(const std::string &object_path, const std::string &interface_name)
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	saftbus::write(get_fd(), saftbus::GET_SAFTBUS_INDEX);
	saftbus::write(get_fd(), object_path);
	saftbus::write(get_fd(), interface_name);
	int saftbus_index = -1;
	saftbus::read(get_fd(), saftbus_index);
	return saftbus_index;
}


int ProxyConnection::get_connection_id()
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	std::istringstream id_in(_saftbus_id.substr(1));
	int id;
	id_in >> id;
	return id;
}

void ProxyConnection::send_signal_flight_time(double signal_flight_time) {
	std::unique_lock<std::mutex> lock(_socket_mutex);
	saftbus::write(get_fd(), saftbus::SIGNAL_FLIGHT_TIME);
	saftbus::write(get_fd(), signal_flight_time);
}


void ProxyConnection::send_proxy_signal_fd(int pipe_fd, 
	                                       std::string object_path,
	                                       std::string interface_name,
	                                       int global_id) 
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	saftbus::write(get_fd(), saftbus::SIGNAL_FD);
	saftbus::sendfd(get_fd(), pipe_fd);	
	saftbus::write(get_fd(), object_path);
	saftbus::write(get_fd(), interface_name);
	saftbus::write(get_fd(), global_id);
}

std::string ProxyConnection::introspect(const std::string &object_path, const std::string &interface_name)
{
	saftbus::write(get_fd(), saftbus::SAFTBUS_CTL_INTROSPECT);
	saftbus::write(get_fd(), object_path);
	saftbus::write(get_fd(), interface_name);
	std::string xml;
	saftbus::read(get_fd(), xml);
	return xml;
}

Serial& ProxyConnection::call_sync (int saftbus_index,
	                                const std::string& object_path, 
	                                const std::string& interface_name, 
	                                const std::string& name, 
	                                const Serial& parameters, 
	                                const std::string& bus_name, 
	                                int timeout_msec)
{
	try {
		std::unique_lock<std::mutex> lock(_socket_mutex);

		Serial message;
		message.put(saftbus_index);
		message.put(object_path);
		message.put(_saftbus_id);
		message.put(interface_name);
		message.put(name);
		message.put(parameters);

		// serialize into a byte stream
		uint32_t size = message.get_size();
		const char *data_ptr =  static_cast<const char*>(message.get_data());
		// write the data into the socket
		saftbus::write(get_fd(), saftbus::METHOD_CALL);
		saftbus::write(get_fd(), size);
		saftbus::write_all(get_fd(), data_ptr, size);

		// receive response from socket
		saftbus::MessageTypeS2C type;
		saftbus::read(get_fd(), type);
		if (type == saftbus::METHOD_ERROR) {
			// this was an error which will be converted into an exception
			saftbus::Error::Type type;
			std::string what;
			saftbus::read(get_fd(), type);
			saftbus::read(get_fd(), what);
			throw saftbus::Error(saftbus::Error::FAILED, what);
		} else if (type == saftbus::METHOD_REPLY) {
			// read regular function return value
			saftbus::read(get_fd(), size);
			_call_sync_result.data().resize(size);
			if (size > 0) {
				saftbus::read_all(get_fd(), &_call_sync_result.data()[0], size);
			}

			return _call_sync_result;
		} else {
			std::ostringstream msg;
			msg << "ProxyConnection::call_sync() : unexpected type " << type << " instead of METHOD_REPLY or METHOD_ERROR";
			throw saftbus::Error(saftbus::Error::FAILED, msg.str());
		}
	} catch(std::exception &e) {
		std::ostringstream msg;
		msg << "ProxyConnection::call_sync() exception: " << e.what() << std::endl;
		msg << object_path << std::endl;
		msg << interface_name << std::endl;
		msg << name << std::endl;
		throw saftbus::Error(saftbus::Error::FAILED, msg.str().c_str());
	}

}


}