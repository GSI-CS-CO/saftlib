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
	std::cerr << "saftbus::ProxyConnection(" << base_name << ")" << std::endl;
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
				throw std::runtime_error("all sockets busy");
			}
			// if there are more attempts left just continue with the next socket filename
		}

		try {
			// see if we can really write and read on the socket...
			saftbus::write(get_fd(), saftbus::SENDER_ID);  // ask the saftd for an ID on the saftbus
			saftbus::read(get_fd(), _saftbus_id);  
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
	                                       Glib::ustring object_path,
	                                       Glib::ustring interface_name,
	                                       int global_id) 
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	saftbus::write(get_fd(), saftbus::SIGNAL_FD);
	saftbus::sendfd(get_fd(), pipe_fd);	
	saftbus::write(get_fd(), object_path);
	saftbus::write(get_fd(), interface_name);
	saftbus::write(get_fd(), global_id);
}

void ProxyConnection::remove_proxy_signal_fd(Glib::ustring object_path,
	                                         Glib::ustring interface_name,
	                                         int global_id) 
{
	std::unique_lock<std::mutex> lock(_socket_mutex);
	saftbus::write(get_fd(), saftbus::SIGNAL_REMOVE_FD);
	saftbus::write(get_fd(), object_path);
	saftbus::write(get_fd(), interface_name);
	saftbus::write(get_fd(), global_id);
}

Glib::VariantContainerBase& ProxyConnection::call_sync (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& name, const Glib::VariantContainerBase& parameters, const Glib::ustring& bus_name, int timeout_msec)
{
	try {
	std::unique_lock<std::mutex> lock(_socket_mutex);
	// perform a remote function call
	// first append message meta informations like: type of message, recipient, sender, interface name
	std::vector<Glib::VariantBase> message;
	message.push_back(Glib::Variant<Glib::ustring>::create(object_path));
	message.push_back(Glib::Variant<Glib::ustring>::create(_saftbus_id));
	message.push_back(Glib::Variant<Glib::ustring>::create(interface_name));
	message.push_back(Glib::Variant<Glib::ustring>::create(name)); // name can be method_name or property_name
	message.push_back(parameters);
	// then convert into a variant vector type
	Glib::Variant<std::vector<Glib::VariantBase> > var_message = Glib::Variant<std::vector<Glib::VariantBase> >::create(message);
	// serialize into a byte stream
	guint32 size = var_message.get_size();
	const char *data_ptr =  static_cast<const char*>(var_message.get_data());
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
		Glib::ustring what;
		saftbus::read(get_fd(), type);
		saftbus::read(get_fd(), what);
		throw Gio::DBus::Error(Gio::DBus::Error::FAILED, what);
	} else if (type == saftbus::METHOD_REPLY) {
		// read regular function return value
		saftbus::read(get_fd(), size);
		_call_sync_result_buffer.resize(size);
		saftbus::read_all(get_fd(), &_call_sync_result_buffer[0], size);

		// deserialize the content into our buffer
		deserialize(_call_sync_result, &_call_sync_result_buffer[0], _call_sync_result_buffer.size());
		return _call_sync_result;					
	} else {
		std::ostringstream msg;
		msg << "ProxyConnection::call_sync() : unexpected type " << type << " instead of METHOD_REPLY or METHOD_ERROR";
		throw Gio::DBus::Error(Gio::DBus::Error::FAILED, msg.str());
	}

	} catch(std::exception &e) {
		std::cerr << "ProxyConnection::call_sync() exception: " << e.what() << std::endl;
		std::cerr << object_path << std::endl;
		std::cerr << interface_name << std::endl;
		std::cerr << name << std::endl;
		throw Gio::DBus::Error(Gio::DBus::Error::FAILED, e.what());
	}

}


void ProxyConnection::call(const Glib::ustring& object_path,
		  const Glib::ustring& interface_name,
		  const Glib::ustring& method_name,
		  const Glib::VariantContainerBase& parameters,
		  const SlotAsyncReady& slot,
		  const Glib::RefPtr< Gio::Cancellable >& cancellable,
		  const Glib::RefPtr< Gio::UnixFDList >& fd_list,
		  const Glib::ustring& bus_name)
{
	// same as call_sync, only we return after passing the args
	try {
		std::unique_lock<std::mutex> lock(_socket_mutex);
		// perform a remote function call
		// first append message meta informations like: type of message, recipient, sender, interface name
		std::vector<Glib::VariantBase> message;
		message.push_back(Glib::Variant<Glib::ustring>::create(object_path));
		message.push_back(Glib::Variant<Glib::ustring>::create(_saftbus_id));
		message.push_back(Glib::Variant<Glib::ustring>::create(interface_name));
		message.push_back(Glib::Variant<Glib::ustring>::create(method_name)); 
		message.push_back(parameters);
		// then convert into a variant vector type
		Glib::Variant<std::vector<Glib::VariantBase> > var_message = Glib::Variant<std::vector<Glib::VariantBase> >::create(message);
		// serialize into a byte stream
		guint32 size = var_message.get_size();
		const char *data_ptr =  static_cast<const char*>(var_message.get_data());
		// write the data into the socket
		saftbus::write(get_fd(), saftbus::METHOD_CALL_ASYNC);
		saftbus::write(get_fd(), size);
		saftbus::write_all(get_fd(), data_ptr, size);
		// write file descriptors from list
		guint32 fd_length = fd_list->get_length();
		saftbus::write(get_fd(), fd_length);
		for (guint32 i = 0; i < fd_length; ++i) {
			int fd = fd_list->get(i);
			saftbus::sendfd(get_fd(), fd);	
			close(fd);
		}


		// ask the thread-default main context to notify us when the call result is delivered ...
		GMainContext *default_context = g_main_context_get_thread_default();
		Glib::RefPtr<Glib::MainContext> context = Glib::wrap(default_context);
		context->reference();
		context->signal_io().connect(sigc::bind(sigc::mem_fun(this, &ProxyConnection::on_async_call_reply), slot, fd_list), get_fd(), Glib::IO_IN | Glib::IO_HUP, Glib::PRIORITY_HIGH);
		// ... and return
	} catch (std::exception &e) {
		std::cerr << "ProxyConnection::call() exception: " << e.what() << std::endl;
		std::cerr << object_path << std::endl;
		std::cerr << interface_name << std::endl;
		std::cerr << method_name << std::endl;
		throw Gio::DBus::Error(Gio::DBus::Error::FAILED, e.what());
	}
}

Glib::VariantContainerBase& ProxyConnection::call_finish(const Glib::RefPtr< Gio::AsyncResult >&res)
{
	try {
		// receive response from socket
		saftbus::MessageTypeS2C type;
		saftbus::read(get_fd(), type);
		if (type == saftbus::METHOD_ERROR) {
			// this was an error which will be converted into an exception
			saftbus::Error::Type type;
			Glib::ustring what;
			saftbus::read(get_fd(), type);
			saftbus::read(get_fd(), what);
			throw Gio::DBus::Error(Gio::DBus::Error::FAILED, what);
		} else if (type == saftbus::METHOD_REPLY) {
			// read regular function return value
			guint32 size;
			saftbus::read(get_fd(), size);
			_call_sync_result_buffer.resize(size);
			saftbus::read_all(get_fd(), &_call_sync_result_buffer[0], size);
			// deserialize the content into our buffer
			deserialize(_call_sync_result, &_call_sync_result_buffer[0], _call_sync_result_buffer.size());
			_result = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(_call_sync_result.get_child(0));
			return _result;					
		} else {
			std::ostringstream msg;
			msg << "ProxyConnection::call_finish() : unexpected type " << type << " instead of METHOD_REPLY or METHOD_ERROR";
			throw Gio::DBus::Error(Gio::DBus::Error::FAILED, msg.str());
		}
	} catch(std::exception &e) {
		std::cerr << "ProxyConnection::call_finish() exception: " << e.what() << std::endl;
		throw Gio::DBus::Error(Gio::DBus::Error::FAILED, e.what());
	}
}

bool ProxyConnection::on_async_call_reply(Glib::IOCondition condition, const SlotAsyncReady& slot, const Glib::RefPtr< Gio::UnixFDList >& fd_list)
{
	// main context informed us that the result is there
	Glib::RefPtr<Gio::AsyncResult> async_result = Glib::RefPtr<Gio::AsyncResult>(); // we don't use this here
	slot(async_result);
	// disconnect from the main context by returning false
	return false;
}





}