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
//#include <giomm/dbuserror.h>

namespace saftbus
{

ProxyConnection::ProxyConnection(const std::string &base_name)
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	//std::cerr << "saftbus::ProxyConnection(" << base_name << ")" << std::endl;
	for (;;) {
		// create a local unix socket
		_create_socket = socket(PF_LOCAL, SOCK_SEQPACKET, 0);
		if (_create_socket <= 0) {
			throw std::runtime_error("ProxyConnection::ProxyConnection() : cannot create socket");
		}
		_address.sun_family = AF_LOCAL;

		// try to open first available socket
		for (int i = 0; i < N_CONNECTIONS; ++i)
		{
			std::ostringstream socket_filename;
			socket_filename << base_name << std::setw(2) << std::setfill('0') << i;
			strcpy(_address.sun_path, socket_filename.str().c_str());
			// try to connect the socket to this 
			int connect_result = connect( _create_socket, (struct sockaddr *)&_address , sizeof(_address));
			if (connect_result == 0) {
				// success! we are done
				_connection_id = i;
				break;
			}
			// failure to connect ...
			
			// check if we failed to connect even on the last socket filename
			if (i == N_CONNECTIONS-1) {
				// no success on all socket files
				throw std::runtime_error("Cannot connect to socket. Possible reasons: all sockets busy, saftd not running, or wrong socket permissions");
			}
			// if there are more attempts left just continue with the next socket filename
		}

		try {
			//std::cerr << "ProxyConnection ask for _saftbus_id" << std::endl;
			// see if we can really write and read on the socket...
			saftbus::write(get_fd(), saftbus::SENDER_ID);  // ask the saftd for an ID on the saftbus
			saftbus::read(get_fd(), _saftbus_id);  
			//std::cerr << "received _saftbus_id " << _saftbus_id << std::endl;
			return;
		}  catch (...) {
			std::cerr << "ProxyConnection::ProxyConnection() threw" << std::endl;
			// ... If above operation failed, our socket was probably assigned to 
			//     another ProxyConnection as well, and we lost that race condition. 
			//     We react with going to the next free socket filename
			continue;
		}
	}
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
	return _connection_id;
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

void ProxyConnection::remove_proxy_signal_fd(std::string object_path,
	                                         std::string interface_name,
	                                         int global_id) 
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	saftbus::write(get_fd(), saftbus::SIGNAL_REMOVE_FD);
	saftbus::write(get_fd(), object_path);
	saftbus::write(get_fd(), interface_name);
	saftbus::write(get_fd(), global_id);
}

Serial& ProxyConnection::call_sync (const std::string& object_path, 
	                                const std::string& interface_name, 
	                                const std::string& name, 
	                                const Serial& parameters, 
	                                const std::string& bus_name, 
	                                int timeout_msec)
{
	try {
	std::unique_lock<std::mutex> lock(_socket_mutex);
	// perform a remote function call
	// first append message meta informations like: type of message, recipient, sender, interface name
	// std::vector<Glib::VariantBase> message;
	// message.push_back(Glib::Variant<std::string>::create(object_path));
	// message.push_back(Glib::Variant<std::string>::create(_saftbus_id));
	// message.push_back(Glib::Variant<std::string>::create(interface_name));
	// message.push_back(Glib::Variant<std::string>::create(name)); // name can be method_name or property_name
	// message.push_back(parameters);
	// // then convert into a variant vector type
	// Glib::Variant<std::vector<Glib::VariantBase> > var_message = Glib::Variant<std::vector<Glib::VariantBase> >::create(message);

	// std::cerr << "ProxyConnection::call_sync(" << object_path << ", " 
	//                                            << interface_name << ", "
	//                                            << name << ")" << std::endl;

	Serial message;
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
		//_call_sync_result_buffer.resize(size);
		_call_sync_result.data().resize(size);
		if (size > 0) {
			saftbus::read_all(get_fd(), &_call_sync_result.data()[0], size);
		}

		// deserialize the content into our buffer
		//deserialize(_call_sync_result, &_call_sync_result_buffer[0], _call_sync_result_buffer.size());

		//std::cerr << "ProxyConnection::call_sync(" << name << ") received Serial" << std::endl;
		//_call_sync_result.print();
		//std::cerr << "ProxyConnection::call_sync(" << name << ") done " << std::endl;
		return _call_sync_result;
	} else {
		std::ostringstream msg;
		msg << "ProxyConnection::call_sync() : unexpected type " << type << " instead of METHOD_REPLY or METHOD_ERROR";
		throw saftbus::Error(saftbus::Error::FAILED, msg.str());
	}

	} catch(std::exception &e) {
		std::cerr << "ProxyConnection::call_sync() exception: " << e.what() << std::endl;
		std::cerr << object_path << std::endl;
		std::cerr << interface_name << std::endl;
		std::cerr << name << std::endl;
		throw saftbus::Error(saftbus::Error::FAILED, e.what());
	}

}


}