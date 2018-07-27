#include "ProxyConnection.h"
#include "Proxy.h"


#include <iostream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <errno.h>
#include <algorithm>

namespace saftbus
{

ProxyConnection::ProxyConnection(const Glib::ustring &base_name)
{
	int _debug_level = 1;
	// try to open first available socket
	_create_socket = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (_create_socket > 0) {
		if (_debug_level) std::cerr << "socket created" << std::endl;
	} // TODO: else { ... }
	_address.sun_family = AF_LOCAL;

	for (int i = 0; i < 100; ++i)
	{
		std::ostringstream name_out;
		name_out << base_name << std::setw(2) << std::setfill('0') << i;
		strcpy(_address.sun_path, name_out.str().c_str());
		if (_debug_level) std::cerr << "try to connect to " << name_out.str() << std::endl;
		int connect_result = connect( _create_socket, (struct sockaddr *)&_address , sizeof(_address));
		if (connect_result == 0) {
			if (_debug_level) std::cerr << "connection established" << std::endl;
			//char msg = 't';
			//int result = send(_create_socket, &msg, 1, MSG_DONTWAIT); 
			//if (_debug_level) std::cerr << "result = " << result << std::endl;
			break;
		} else {
			if (_debug_level) std::cerr << "connect_result = " << connect_result << std::endl;
			if (connect_result == -1) {
				if (_debug_level) std::cerr << "  errno = " << errno << " -> " << strerror(errno) << std::endl;
				switch(errno) {
					case ECONNREFUSED:
						continue;
					break;
					case ENOENT:
						throw std::runtime_error("all sockets busy");
					break;
					default:
						throw std::runtime_error("unknown error");
				}
			} 
		}
	}
    Glib::signal_io().connect(sigc::mem_fun(*this, &ProxyConnection::dispatch), _create_socket, Glib::IO_IN | Glib::IO_HUP, Glib::PRIORITY_HIGH);
}


// guint ProxyConnection::register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable)
// {
// 	std::cerr << "ProxyConnection::register_object("<< object_path <<") called" << std::endl;
// 	//_saftbus_objects[object_path] = counter++;
// 	guint result = _saftbus_objects.size();
// 	SaftbusObject object(object_path, interface_info, vtable);
// 	_saftbus_objects.push_back(object);
// 	// for(auto iter = _saftbus_objects.begin(); iter != _saftbus_objects.end(); ++iter)
// 	// {
// 	// 	std::cerr << *iter << std::endl;
// 	// }
// 	return result;
// }
// bool ProxyConnection::unregister_object (guint registration_id)
// {
// 	if (registration_id < _saftbus_objects.size())
// 	{
// 		_saftbus_objects[registration_id].object_path = "";
// 		return true;
// 	}
// 	return false;
// }


guint ProxyConnection::signal_subscribe(const SlotSignal&  	slot,
										const Glib::ustring&  	sender,
										const Glib::ustring&  	interface_name,
										const Glib::ustring&  	member,
										const Glib::ustring&  	object_path,
										const Glib::ustring&  	arg0//,
										)//SignalFlags  	flags)
{
	std::cerr << "ProxyConnection::signal_subscribe(" << sender << "," << interface_name << "," << member << "," << object_path << ") called" << std::endl;
	return 0;
}

void ProxyConnection::signal_unsubscribe(guint subscription_id)
{
	std::cerr << "ProxyConnection::signal_unsubscribe() called" << std::endl;
}


// void 	ProxyConnection::emit_signal (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name, const Glib::VariantContainerBase& parameters)
// {
// 	std::cerr << "ProxyConnection::emit_signal(" << object_path << "," << interface_name << "," << signal_name << "," << destination_bus_name << ") called" << std::endl;
// 	for (unsigned n = 0; n < parameters.get_n_children(); ++n)
// 	{
// 		Glib::VariantBase child;
// 		parameters.get_child(child, n);
// 		std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << std::endl;
// 	}
// }


Glib::VariantContainerBase ProxyConnection::call_sync (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& method_name, const Glib::VariantContainerBase& parameters, const Glib::ustring& bus_name, int timeout_msec)
{
	std::cerr << "ProxyConnection::call_sync(" << object_path << "," << interface_name << "," << method_name << ") called" << std::endl;
	return Glib::VariantContainerBase();
}



bool ProxyConnection::dispatch(Glib::IOCondition condition) 
{
	int _debug_level = 1;
	if (_debug_level) std::cerr << "ClientConnection::dispatch() called" << std::endl;


	return true;
}




}
