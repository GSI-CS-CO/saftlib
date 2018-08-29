#include "Connection.h"
#include "Socket.h"
#include "core.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>


namespace saftbus
{

int Connection::_saftbus_id_counter = 0;

Connection::Connection(int number_of_sockets, const std::string& base_name)
	: _saftbus_object_id_counter(1)
	, _saftbus_signal_handle_counter(1)
{
	for (int i = 0; i < number_of_sockets; ++i)
	{
		std::ostringstream name_out;
		name_out << base_name << std::setw(2) << std::setfill('0') << i;
		_sockets.push_back(std::shared_ptr<Socket>(new Socket(name_out.str(), this)));
	}
}
// Add an object to the set of saftbus controlled objects. Each registered object is assigned a unique ID.
// The ID is returned by this function to Saftlib and is used later to unregister the object again.
// saftbus Proxies address saftbus objects by their object_path and interface_name.
// their object_path and interface_name and provide a vtable (a table with functions for 
// "method_call" "set_property" and "get_property"). The saftbus routes all redirects function
// calles to one of the registered object's vtable.
// Saftbus objects have unique IDs, object_path and interface name serve to address the objects from outside (i.e. from Proxies).
guint Connection::register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable)
{
	++_saftbus_object_id_counter;
	if (_debug_level > 3) std::cerr << "LLLLLLLLLLLLLLLLLLL ****************** Connection::register_object("<< object_path <<") called . id = " << _saftbus_object_id_counter << std::endl;
	//guint result = _saftbus_objects.size();
	guint result = _saftbus_object_id_counter;
	_saftbus_objects[_saftbus_object_id_counter] = std::shared_ptr<InterfaceVTable>(new InterfaceVTable(vtable));
	Glib::ustring interface_name = interface_info->get_interface_name();
	if (_debug_level > 4) std::cerr << "    got interface_name = " << interface_name << std::endl;
	_saftbus_indices[interface_name][object_path] = result;
	return result;
}
bool Connection::unregister_object (guint registration_id)
{
	if (_debug_level > 2) std::cerr << "MMMMMMMMMMMMMMMMMMM ****************** Connection::unregister_object("<< registration_id <<") called" << std::endl;

	_saftbus_objects.erase(registration_id);
	return true;
	//list_all_resources();
}


// subscribe to local signal (local inside the saftbus process)
// all registered signals will be redirected to the saftbus_objects. These are of type Owned. 
// @param slot is the function that will be called when a signal arrives
guint Connection::signal_subscribe(const SlotSignal& slot,
								   const Glib::ustring& sender,
								   const Glib::ustring& interface_name,
								   const Glib::ustring& member,
								   const Glib::ustring& object_path,
								   const Glib::ustring& arg0//,
								   )//SignalFlags  	flags)
{
	if (_debug_level > 2) std::cerr << "Connection::signal_subscribe(" << sender << "," << interface_name << "," << member << "," << object_path << ", " << arg0 << ") called" << std::endl;

	// return result;
	++_saftbus_signal_handle_counter;
	_handle_to_signal_map[_saftbus_signal_handle_counter].connect(slot);

	_id_handles_map[arg0].insert(_saftbus_signal_handle_counter);

	if (_debug_level > 3) {
		for(auto it = _id_handles_map.begin(); it != _id_handles_map.end(); ++it) {
			for (auto i = it->second.begin(); i != it->second.end(); ++i) {
				std::cerr << "_id_handles_map[" << it->first << "]:" << *i << std::endl;
			}
		}
	}
	if (_debug_level > 2) std::cerr << "return signal handle " << _saftbus_signal_handle_counter << std::endl;
	return _saftbus_signal_handle_counter;
}

void Connection::signal_unsubscribe(guint subscription_id) 
{
	if (_debug_level > 3) std::cerr << "Connection::signal_unsubscribe(" << subscription_id << ") called" << std::endl;
	if (_debug_level > 4) {
		for(auto it = _handle_to_signal_map.begin(); it != _handle_to_signal_map.end(); ++it) {
			std::cerr << "_owned_signals[" << it->first << "]" << std::endl;
		}
	}
	//std::cerr << "list to erase handle " << subscription_id << std::endl;
	_erased_handles.insert(subscription_id);
}

void Connection::handle_disconnect(Socket *socket)
{
	// this is called when the communication partner a particular socket (given as function argument) 
	// disappeared. In that case we have to close the connection and destroy all objects 
	if (_debug_level > 2) std::cerr << "Connection::handle_disconnect() called" << std::endl;
	Glib::VariantContainerBase arg;
	Glib::ustring& saftbus_id = socket->saftbus_id();
	socket->close_connection();
	socket->wait_for_client();

	// only ouput
	if (_id_handles_map.find(saftbus_id) != _id_handles_map.end()) { // we have to emit the quit signals
		for(auto it = _id_handles_map[saftbus_id].begin(); it != _id_handles_map[saftbus_id].end(); ++it) {
			//std::cerr << "_handle_to_signal_map[" << *it << "]  " << std::endl;
		}
	}

	if (_debug_level > 2) std::cerr << "collect unsubscription handles" << std::endl;
	// emit the signals
	if (_id_handles_map.find(saftbus_id) != _id_handles_map.end()) { // we have to emit the quit signals
		for(auto it = _id_handles_map[saftbus_id].begin(); it != _id_handles_map[saftbus_id].end(); ++it) {
			// a signal should only be sent if (*it) is not in the list of _erased_handles 
			if (_erased_handles.find(*it) == _erased_handles.end()) {
				if (_debug_level > 2) std::cerr << "sending signal to clean up: " << *it << std::endl;
				_handle_to_signal_map[*it].emit(saftbus::connection, "", "", "", "" , arg);
			}
		}
	}

	//std::cerr << "delete unsubscribed handles" << std::endl;
	// see which signals were unsubscribed
	for (auto it = _erased_handles.begin(); it != _erased_handles.end(); ++it) {
		//std::cerr << "erase handle " << *it << std::endl;
		_handle_to_signal_map.erase(*it);
	}
	_erased_handles.clear();
	_id_handles_map.erase(saftbus_id);



	clean_all_fds_from_socket(socket);

	if (_debug_level > 2) print_all_fds();
	if (_debug_level > 2) std::cerr << "Connection::handle_disconnect() done" << std::endl;
}


double delta_t(struct timespec start, struct timespec stop) 
{
	return (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
         - (1.0e6*start.tv_sec   + 1.0e-3*start.tv_nsec);
}

// send a signal to all connected sockets
// I think I would be possible to filter the signals and send them only to sockets where they are actually expected
void Connection::emit_signal(const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name, const Glib::VariantContainerBase& parameters)
{
    struct timespec start_time, packing_done_time, serialized_time;
    clock_gettime( CLOCK_REALTIME, &start_time);

	if (_debug_level > 4) std::cerr << "Connection::emit_signal(" << object_path << "," << interface_name << "," << signal_name << "," << parameters.print() << ") called" << std::endl;
		// std::cerr << "   signals" << std::endl;
		// for(auto it = _owned_signals_signatures.begin() ; it != _owned_signals_signatures.end(); ++it) 
		// 		std::cerr << "     " << *it << std::endl;
	if (_debug_level > 5) {
		for (unsigned n = 0; n < parameters.get_n_children(); ++n)
		{
			Glib::VariantBase child;
			parameters.get_child(child, n);
			std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
		}
	}


	std::vector<Glib::VariantBase> signal_msg;
	signal_msg.push_back(Glib::Variant<Glib::ustring>::create(object_path));
	signal_msg.push_back(Glib::Variant<Glib::ustring>::create(interface_name));
	signal_msg.push_back(Glib::Variant<Glib::ustring>::create(signal_name));
	signal_msg.push_back(Glib::Variant<gint64>::create(start_time.tv_sec));
	signal_msg.push_back(Glib::Variant<gint64>::create(start_time.tv_nsec));
	signal_msg.push_back(parameters);
	Glib::Variant<std::vector<Glib::VariantBase> > var_signal_msg = Glib::Variant<std::vector<Glib::VariantBase> >::create(signal_msg);

    clock_gettime( CLOCK_REALTIME, &packing_done_time);

	if (_debug_level > 5) std::cerr << "signal message " << var_signal_msg.get_type_string() << " " << var_signal_msg.print() << std::endl;
	guint32 size = var_signal_msg.get_size();
	if (_debug_level > 5) std::cerr << " size of signal is " << size << std::endl;
	const char *data_ptr = static_cast<const char*>(var_signal_msg.get_data());

    clock_gettime( CLOCK_REALTIME, &serialized_time);

	try {  
		// it may happen that the Proxy that is supposed to read the signal
		// does not have a running MainLoop. In this case the pipe will become full
		// at some point and the write function will throw. We have to catch that
		// to prevent the saft daemon from crashing


		if (signal_name == "PropertiesChanged") { // special case for property changed signals
			Glib::VariantContainerBase* params = const_cast<Glib::VariantContainerBase*>(&parameters);
			Glib::Variant<Glib::ustring> derived_interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(params->get_child(0));
			if (_debug_level > 2) std::cerr << "PropertiesChaned ----> derived_interface_name = " << derived_interface_name.get() << " " << object_path << std::endl;

			// directly send signal
			std::set<ProxyPipe> &setProxyPipe = _proxy_pipes[derived_interface_name.get()][object_path];
			for (auto i = setProxyPipe.begin(); i != setProxyPipe.end(); ++i) {
				bool error = false;
				if (_debug_level > 2) std::cerr << "sending signal through pipe " << i->fd << std::endl;
				if (write(i->fd, saftbus::SIGNAL) < 0) error = true;
				if (write(i->fd, size) < 0 ) error = true;
				if (write_all(i->fd, data_ptr, size) < 0 ) error = true;
				if (_debug_level > 2) std::cerr << "done with signal through pipe" << std::endl;
				if (error) std::cerr << "there was an error sending the Properties Changed signal" << std::endl;
			}

		} else {
			if (_debug_level > 2) std::cerr << "Normal Signal ----> interface_name = " << interface_name << " " << object_path << std::endl;

			// directly send signal
			std::set<ProxyPipe> &setProxyPipe = _proxy_pipes[interface_name][object_path];
			for (auto i = setProxyPipe.begin(); i != setProxyPipe.end(); ++i) {
				bool error = false;
				if (_debug_level > 2) std::cerr << "sending signal through pipe " << i->fd  << std::endl;
				if (write(i->fd, saftbus::SIGNAL) < 0 ) error = true;
				if (write(i->fd, size) < 0 ) error = true;
				if (write_all(i->fd, data_ptr, size) < 0 ) error = true;
				if (_debug_level > 2) std::cerr << "done with signal through pipe" << std::endl;
				if (error) std::cerr << "there was an error sending the (real) signal" << std::endl;
			}
		}
	} catch (...) {
		//std::cerr << "caught a signal (Pipe full? Proxy without Glib::MainLoop?)" << std::endl;
	}

	//std::cerr << "signal packing time = " << delta_t(start_time, packing_done_time) << " us" << std::endl;
	//std::cerr << "serialization  time = " << delta_t(packing_done_time, serialized_time) << " us" << std::endl;
	//list_all_resources();
}



bool Connection::dispatch(Glib::IOCondition condition, Socket *socket) 
{
	try {
		static int cnt = 0;
		++cnt;
		if (_debug_level > 2)  std::cerr << "Connection::dispatch() called one socket[" << socket_nr(socket) << "]" << std::endl;
		MessageTypeC2S type;
		int result = saftbus::read(socket->get_fd(), type);
		if (result == -1) {
			if (_debug_level > 2) std::cerr << "client disconnected" << std::endl;
			handle_disconnect(socket);
		} else {
			switch(type)
			{
				case saftbus::SAFTBUS_CTL_HELLO:
				{
					std::cerr << "got signal from saftbus-ctl tool" << std::endl;
				}
				break;
				case saftbus::SAFTBUS_CTL_STATUS:
				{
					saftbus::write(socket->get_fd(), _saftbus_indices);
					std::vector<int> indices;
					for (auto it: _saftbus_objects) {
						indices.push_back(it.first);
					}
					saftbus::write(socket->get_fd(), indices);
					saftbus::write(socket->get_fd(), _signal_flight_times);
				}
				break;
				case saftbus::SENDER_ID:
				{
					if (_debug_level > 2) std::cerr << "Connection::dispatch() SENDER_ID received" << std::endl;
					Glib::ustring sender_id;
					saftbus::read(socket->get_fd(), sender_id);
					// ignore the suggested sender_id from the client and generate a new, unique id
					++_saftbus_id_counter; // generate a new id value (increasing numbers)
					std::ostringstream id_out;
					id_out << ":" << _saftbus_id_counter;
					socket->saftbus_id() = id_out.str();
					// send the id to the ProxyConnection
					saftbus::write(socket->get_fd(), socket->saftbus_id());
				}
				case saftbus::REGISTER_CLIENT: 
				{
				}
				break;
				case saftbus::SIGNAL_FLIGHT_TIME: 
				{
					double dt;
					read(socket->get_fd(), dt);
					int dt_us = dt;
					++_signal_flight_times[dt_us];
					if (dt > 600) {
						std::cerr << "saftd: long signal flight time detected: " << dt << " us" << std::endl; 
					}
					//std::cerr << "signal flight time reported: " << dt << " us" << std::endl;
				}
				break;
				case saftbus::SIGNAL_FD: 
				{
					if (_debug_level > 2) std::cerr << "Connection::dispatch() received SIGNAL_FD" << std::endl;
					int fd = saftbus::recvfd(socket->get_fd());
					Glib::ustring name, object_path, interface_name;
					int proxy_id;
					read(socket->get_fd(), object_path);
					read(socket->get_fd(), interface_name);
					read(socket->get_fd(), proxy_id);
					if (_debug_level > 2) std::cerr << "Connection::dispatch() got fd " << fd << " " << name << " " << object_path << " " << interface_name << " " << proxy_id << std::endl;

					int flags = fcntl(fd, F_GETFL, 0);
					fcntl(fd, F_SETFL, flags | O_NONBLOCK);

					ProxyPipe pp;
					pp.id = proxy_id;
					pp.fd = fd;
					pp.socket_nr = socket_nr(socket);
					_proxy_pipes[interface_name][object_path].insert(pp);

					// int msg = 42;
					// write(fd, msg);
				}
				break;
				case saftbus::SIGNAL_REMOVE_FD: 
				{
					if (_debug_level > 2) std::cerr << "Connection::dispatch() received SIGNAL_REMOVE_FD" << std::endl;
					if (_debug_level > 2) print_all_fds();
					Glib::ustring name, object_path, interface_name;
					int proxy_id;
					read(socket->get_fd(), object_path);
					read(socket->get_fd(), interface_name);
					read(socket->get_fd(), proxy_id);

					ProxyPipe pp;
					pp.id = proxy_id;
					auto pp_done = _proxy_pipes[interface_name][object_path].find(pp);
					close(pp_done->fd);
					_proxy_pipes[interface_name][object_path].erase(pp_done);
					if (_debug_level > 2) std::cerr << "Connection::dispatch() after erasing pipe fd" << std::endl;
					if (_debug_level > 2) print_all_fds();
				}
				break;
				case saftbus::METHOD_CALL: 
				{
					if (_debug_level > 2) std::cerr << "Connection::dispatch() METHOD_CALL received" << std::endl;
					guint32 size;
					saftbus::read(socket->get_fd(), size);
					if (_debug_level > 5) std::cerr << "expecting message with size " << size << std::endl;
					std::vector<char> buffer(size);
					saftbus::read_all(socket->get_fd(), &buffer[0], size);
					Glib::Variant<std::vector<Glib::VariantBase> > payload;
					deserialize(payload, &buffer[0], buffer.size());
					Glib::Variant<Glib::ustring> object_path    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(0));
					Glib::Variant<Glib::ustring> sender         = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(1));
					Glib::Variant<Glib::ustring> interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(2));
					Glib::Variant<Glib::ustring> name           = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(3));
					Glib::VariantContainerBase parameters       = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>   (payload.get_child(4));
					if (_debug_level > 5) std::cerr << "sender = " << sender.get() <<  "    name = " << name.get() << "   object_path = " << object_path.get() <<  "    interface_name = " << interface_name.get() << std::endl;
					if (_debug_level > 5) std::cerr << "parameters = " << parameters.print() << std::endl;
					if (interface_name.get() == "org.freedesktop.DBus.Properties") { // property get/set method call
						if (_debug_level > 5) std::cerr << " interface for Set/Get property was called" << std::endl;

						Glib::Variant<Glib::ustring> derived_interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(0));
						Glib::Variant<Glib::ustring> property_name          = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(1));

						if (_debug_level > 5) std::cerr << "property_name = " << property_name.get() << "   derived_interface_name = " << derived_interface_name.get() << std::endl;

						for (unsigned n = 0; n < parameters.get_n_children(); ++n)
						{
							Glib::VariantBase child;
							parameters.get_child(child, n);
							if (_debug_level > 5) std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
						}					
						int index = _saftbus_indices[derived_interface_name.get()][object_path.get()];
						if (_debug_level > 5) std::cerr << "found saftbus object at index " << index << std::endl;
						
						if (name.get() == "Get") {
							Glib::VariantBase result;
							_saftbus_objects[index]->get_property(result, saftbus::connection, sender.get(), object_path.get(), derived_interface_name.get(), property_name.get());
							if (_debug_level > 2) std::cerr << "getting property " << result.get_type_string() << " " << result.print() << std::endl;
							if (_debug_level > 5) std::cerr << "result is: " << result.get_type_string() << "   .value = " << result.print() << std::endl;

							std::vector<Glib::VariantBase> response;
							response.push_back(result);
							Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

							if (_debug_level > 5) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
							size = var_response.get_size();
							if (_debug_level > 5) std::cerr << " size of response is " << size << std::endl;
							const char *data_ptr = static_cast<const char*>(var_response.get_data());
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);

						} else if (name.get() == "Set") {
							if (_debug_level > 2) std::cerr << "setting property" << std::endl;
							Glib::Variant<Glib::VariantBase> par2 = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::VariantBase> >(parameters.get_child(2));
							if (_debug_level > 5) std::cerr << "par2 = " << par2.get_type_string() << " "<< par2.print() << std::endl;
							// Glib::Variant<bool> vb = Glib::Variant<bool>::create(true);
							// std::cerr << "vb = " << vb.get_type_string() << " " << vb.print() << std::endl;

							//std::cerr << "value = " << value << std::endl;
							// if (_debug_level > 5) std::cerr << " value = " << value.get_type_string() << " " << value.print() << std::endl;
							bool result = _saftbus_objects[index]->set_property(saftbus::connection, sender.get(), object_path.get(), derived_interface_name.get(), property_name.get(), par2.get());

							std::vector<Glib::VariantBase> response;
							response.push_back(Glib::Variant<bool>::create(result));
							Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

							if (_debug_level > 5) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
							size = var_response.get_size();
							if (_debug_level > 5) std::cerr << " size of response is " << size << std::endl;
							const char *data_ptr = static_cast<const char*>(var_response.get_data());
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);
						}

					}
					else // normal method calls 
					{
						if (_debug_level > 2) std::cerr << "a real method call: " << std::endl;
						int index = _saftbus_indices[interface_name.get()][object_path.get()];
						if (_debug_level > 5) std::cerr << "found saftbus object at index " << index << std::endl;
						Glib::RefPtr<MethodInvocation> method_invocation_rptr(new MethodInvocation);
						if (_debug_level > 5) std::cerr << "saftbus::connection.get() " << static_cast<bool>(saftbus::connection) << std::endl;
						if (_debug_level > 2) std::cerr << "doing the function call" << std::endl;
						_saftbus_objects[index]->method_call(saftbus::connection, sender.get(), object_path.get(), interface_name.get(), name.get(), parameters, method_invocation_rptr);
						if (_debug_level > 2) std::cerr << "function call done, getting result" << std::endl;
						Glib::VariantContainerBase result;
						result = method_invocation_rptr->get_return_value();
						if (_debug_level > 2) std::cerr << "result is " << result.get_type_string() << " " << result.print() << std::endl;

						std::vector<Glib::VariantBase> response;
						response.push_back(result);
						Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

						if (_debug_level > 5) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
						size = var_response.get_size();
						const char *data_ptr = static_cast<const char*>(var_response.get_data());
						saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
						saftbus::write(socket->get_fd(), size);
						saftbus::write_all(socket->get_fd(), data_ptr, size);

					}
				}
				break;

				default:
					if (_debug_level > 5) std::cerr << "Connection::dispatch() unknown message type: " << type << std::endl;
					return false;
				break;				
			}

			return true;
		}
	} catch(std::exception &e) {
		std::cerr << "exception in Connection::dispatch(): " << e.what() << std::endl;
		if (_debug_level > 5) print_all_fds();
		handle_disconnect(socket);
		if (_debug_level > 5) print_all_fds();
	}
	return false;
}

void Connection::print_all_fds()
{
	std::cerr << "-------all pipe fds-------" << std::endl;
	std::cerr 
	          << std::setw(5) << "sock" 
			  << std::setw(35) << "interface_name"
	          << std::setw(35) << "object_path"
	          << std::setw(5) << "id"
	          << std::setw(5) << "fd"
	          << std::endl;
	std::cerr << "-----------------------------------------------------------------------------------------------" << std::endl;
	for (auto iter = _proxy_pipes.begin(); iter != _proxy_pipes.end(); ++iter) {
		for (auto itr = iter->second.begin(); itr != iter->second.end(); ++itr) {
			for (auto it = itr->second.begin(); it != itr->second.end(); ++it) {
				std::cerr 
				          << std::setw(5) << it->socket_nr 
						  << std::setw(35) << iter->first 
				          << std::setw(35) << itr->first
				          << std::setw(5) << it->id 
				          << std::setw(5) << it->fd
				          << std::endl;
			}
		}
	}
	std::cerr << "--------------------------" << std::endl;

}

void Connection::clean_all_fds_from_socket(Socket *socket)
{
	int nr = socket_nr(socket);
	for (auto iter = _proxy_pipes.begin(); iter != _proxy_pipes.end(); ++iter) {
		for (auto itr = iter->second.begin(); itr != iter->second.end(); ++itr) {
			std::vector<std::set<ProxyPipe>::iterator > erase_iters;
			for (auto it = itr->second.begin(); it != itr->second.end(); ++it) {
				if (nr == it->socket_nr) {
					erase_iters.push_back(it);
					close(it->fd);
				}
			}
			for(auto it = erase_iters.begin(); it != erase_iters.end(); ++it) {
				itr->second.erase(*it);
			}
		}
	}

}

void Connection::list_all_resources()
{
	std::cerr << "----- saftbus_objects -----" << std::endl;
	std::cerr << "    " << std::setw(5) << "idx" << "  -> " 
						<< std::setw(35) << "object_path"
						<< " , " 
						<< std::setw(35) << "interface_name"
						<< std::setw(5) << "owner"
						<< std::endl;
	std::cerr << "-----------------------------------------------------------------------------------------------" << std::endl;

	for(auto itr : _saftbus_objects) {
		// reverse look-up
		for (auto itr_interface_name : _saftbus_indices) {
			for (auto itr_object_path : itr_interface_name.second) {
				if (itr_object_path.second == itr.first) {
					std::cerr << "    " << std::setw(5) << itr.first << "  -> " 
										<< std::setw(35) << itr_object_path.first 
										<< " , " 
										<< std::setw(35) << itr_interface_name.first 
										<< std::setw(5) << "??"
										<< std::endl;
				}
			}
		}
	}


	print_all_fds();

	// id handles and owner
	std::cerr << "  ----- id handles and owners -----" << std::endl;
	std::cerr << "    " << std::setw(10) << "owner" << std::setw(10) << "handle" << std::endl;
	std::cerr << "-----------------------------------------------------------------------------------------------" << std::endl;
	for (auto itr: _id_handles_map) {
		for (auto it: itr.second) {
			std::cerr << "    " << std::setw(10) << itr.first << std::setw(10) << it << std::endl;
		}
	}

}


int Connection::socket_nr(Socket *socket)
{
	for (guint32 i = 0; i < _sockets.size(); ++i) {
		if (_sockets[i].get() == socket)
			return i;
	}
	return -1;
}


}
