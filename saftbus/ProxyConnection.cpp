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
#include <giomm/dbuserror.h>

namespace saftbus
{

ProxyConnection::ProxyConnection(const Glib::ustring &base_name)
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	for (;;) {
		int _debug_level = 1;
		// create a local unix socket
		_create_socket = socket(PF_LOCAL, SOCK_SEQPACKET, 0);
		if (_create_socket > 0) {
			if (_debug_level > 5) std::cerr << "socket created" << std::endl;
		} else { 
			throw std::runtime_error("cannot create socket");
		}
		_address.sun_family = AF_LOCAL;

		// try to open first available socket
		int i;
		for (i = 0; i < N_CONNECTIONS; ++i)
		{
			std::ostringstream name_out;
			name_out << base_name << std::setw(2) << std::setfill('0') << i;
			strcpy(_address.sun_path, name_out.str().c_str());
			if (_debug_level > 5) std::cerr << "try to connect to " << name_out.str() << std::endl;
			int connect_result = connect( _create_socket, (struct sockaddr *)&_address , sizeof(_address));
			if (connect_result == 0) {
				_connection_id = i;
				break;
			} else {
				if (_debug_level > 5) std::cerr << "connect_result = " << connect_result << std::endl;
				if (connect_result == -1) {
					if (_debug_level > 5) std::cerr << "  errno = " << errno << " -> " << strerror(errno) << std::endl;
					continue;
				} 
			}
		}

		if (i == N_CONNECTIONS) {
			throw std::runtime_error("all sockets busy");
		}

		try {
			write(get_fd(), saftbus::SENDER_ID);  // ask the server to give us an ID
			read(get_fd(), _saftbus_id);
			return;
		}  catch (...) {
			continue;
		}
	}
}


int ProxyConnection::get_connection_id()
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	return _connection_id;
}


Glib::VariantContainerBase& ProxyConnection::call_sync (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& name, const Glib::VariantContainerBase& parameters, const Glib::ustring& bus_name, int timeout_msec)
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	if (_debug_level > 5) std::cerr << "ProxyConnection::call_sync(" << object_path << "," << interface_name << "," << name << ") called" << std::endl;
	// first append message meta informations like: type of message, recipient, sender, interface name
	std::vector<Glib::VariantBase> message;
	message.push_back(Glib::Variant<Glib::ustring>::create(object_path));
	message.push_back(Glib::Variant<Glib::ustring>::create(_saftbus_id));
	message.push_back(Glib::Variant<Glib::ustring>::create(interface_name));
	message.push_back(Glib::Variant<Glib::ustring>::create(name)); // name can be method_name or property_name
	message.push_back(parameters);
	// then convert inot a variant vector type
	Glib::Variant<std::vector<Glib::VariantBase> > var_message = Glib::Variant<std::vector<Glib::VariantBase> >::create(message);
	// serialize
	guint32 size = var_message.get_size();
	const char *data_ptr =  static_cast<const char*>(var_message.get_data());
	// send over socket
	saftbus::write(get_fd(), saftbus::METHOD_CALL);
	saftbus::write(get_fd(), size);
	saftbus::write_all(get_fd(), data_ptr, size);

	// receive from socket
	saftbus::MessageTypeS2C type;
	read(get_fd(), type);
	if (type == saftbus::METHOD_ERROR) {
		saftbus::Error::Type type;
		Glib::ustring what;
		saftbus::read(get_fd(), type);
		saftbus::read(get_fd(), what);
		throw Gio::DBus::Error(Gio::DBus::Error::FAILED, what);
	} else if (type == saftbus::METHOD_REPLY) {
		if (_debug_level > 5) std::cerr << "got response " << type << std::endl;
		saftbus::read(get_fd(), size);
		_call_sync_result_buffer.resize(size);
		saftbus::read_all(get_fd(), &_call_sync_result_buffer[0], size);

		deserialize(_call_sync_result, &_call_sync_result_buffer[0], _call_sync_result_buffer.size());

		if (_debug_level > 5) std::cerr << "ProxyConnection::call_sync respsonse = " << _call_sync_result.print() << std::endl;

		return _call_sync_result;					
	} else {
		std::ostringstream msg;
		msg << " unexpected type " << type << " instead of METHOD_REPLY or METHOD_ERROR";
		throw Gio::DBus::Error(Gio::DBus::Error::FAILED, msg.str());
	}
}

}
