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
	, logger("/tmp/saftbus_connection.log")
{
	for (int i = 0; i < number_of_sockets; ++i)
	{
		std::ostringstream name_out;
		name_out << base_name << std::setw(2) << std::setfill('0') << i;
		_sockets.push_back(std::shared_ptr<Socket>(new Socket(name_out.str(), this)));
	}
}

Connection::~Connection()
{
	logger.newMsg(0).add("Connection::~Connection()\n").log();
}



// Add an object to the set of saftbus controlled objects. Each registered object is assigned a unique ID.
// The ID is returned by this function to Saftlib and is used later to unregister the object again.
// saftbus Proxies address saftbus objects by their object_path and interface_name.
// their object_path and interface_name and provide a vtable (a table with functions for 
// "method_call" "set_property" and "get_property"). The saftbus routes all redirects function
// calles to one of the registered object's vtable.
// Saftbus objects have unique IDs, object_path and interface name serve to address the objects from outside (i.e. from Proxies).
guint Connection::register_object (const Glib::ustring& object_path, 
								   const Glib::RefPtr< InterfaceInfo >& interface_info, 
								   const InterfaceVTable& vtable)
{
	++_saftbus_object_id_counter;
	logger.newMsg(0).add("Connection::register_object(").add(object_path).add(")\n");
	//guint result = _saftbus_objects.size();
	guint result = _saftbus_object_id_counter;
	_saftbus_objects[_saftbus_object_id_counter] = std::shared_ptr<InterfaceVTable>(new InterfaceVTable(vtable));
	Glib::ustring interface_name = interface_info->get_interface_name();
	logger.add(" interface_name:").add(interface_name).add(" -> id:").add(result).log();
	_saftbus_indices[interface_name][object_path] = result;
	return result;
}
bool Connection::unregister_object (guint registration_id)
{
	logger.newMsg(0).add("Connection::unregister_object(").add(registration_id).add(")\n").log();

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
	saftbus::Timer f_time(_function_run_times["Connection::signal_subscribe"]);
	logger.newMsg(0).add("Connection::signal_subscribe(")
	                .add(sender).add(",")
	                .add(interface_name).add(",")
	                .add(member).add(",")
	                .add(object_path).add(",")
	                .add(arg0)
	                .add(")\n");

	// return result;
	++_saftbus_signal_handle_counter;
	_handle_to_signal_map[_saftbus_signal_handle_counter].connect(slot);

	_id_handles_map[arg0].insert(_saftbus_signal_handle_counter);

	for(auto it: _id_handles_map) {
		for (auto i: it.second) {
			logger.add("     _id_handles_map[").add(it.first).add("]: ").add(i).add("\n");
		}
	}
	logger.add("            return signal handle ").add(_saftbus_signal_handle_counter).log();
	return _saftbus_signal_handle_counter;
}

void Connection::signal_unsubscribe(guint subscription_id) 
{
	logger.newMsg(0).add("signal_unsubscribe(").add(subscription_id).add(")\n");
	saftbus::Timer f_time(_function_run_times["Connection::signal_unsubscribe"]);
	for(auto it: _handle_to_signal_map) {
		logger.add("       _owned_signals[").add(it.first).add("]\n");
	}
	logger.log();
	_erased_handles.insert(subscription_id);
}

void Connection::handle_disconnect(Socket *socket)
{
	saftbus::Timer f_time(_function_run_times["Connection::handle_disconnect"]);
	// this is called when the communication partner a particular socket (given as function argument) 
	// disappeared. In that case we have to close the connection and destroy all objects 
	//if (_debug_level > 2) std::cerr << "Connection::handle_disconnect() called" << std::endl;
	logger.newMsg(0).add("Connection::handle_disconnect()\n");
	Glib::VariantContainerBase arg;
	Glib::ustring& saftbus_id = socket->saftbus_id();
	socket->close_connection();
	socket->wait_for_client();

	if (_id_handles_map.find(saftbus_id) != _id_handles_map.end()) { // we have to emit the quit signals
		for(auto it = _id_handles_map[saftbus_id].begin(); it != _id_handles_map[saftbus_id].end(); ++it) {
			//std::cerr << "_handle_to_signal_map[" << *it << "]  " << std::endl;
			logger.add("      _handle_to_signal_map[").add(*it).add("]\n");
		}
	}

	//if (_debug_level > 2) std::cerr << "collect unsubscription handles" << std::endl;
	logger.add("   collect unsubscription handles:\n");
	// emit the signals
	if (_id_handles_map.find(saftbus_id) != _id_handles_map.end()) { // we have to emit the quit signals
		for(auto it = _id_handles_map[saftbus_id].begin(); it != _id_handles_map[saftbus_id].end(); ++it) {
			// a signal should only be sent if (*it) is not in the list of _erased_handles 
			if (_erased_handles.find(*it) == _erased_handles.end()) {
				//if (_debug_level > 2) std::cerr << "sending signal to clean up: " << *it << std::endl;
				logger.add("   sending signal to clean up: ").add(*it).add("\n");
				_handle_to_signal_map[*it].emit(saftbus::connection, "", "", "", "" , arg);
			}
		}
	}

	logger.add("   delete unsubscription handles\n");
	//std::cerr << "delete unsubscribed handles" << std::endl;
	// see which signals were unsubscribed
	for (auto it = _erased_handles.begin(); it != _erased_handles.end(); ++it) {
		logger.add("    erase handle: ").add(*it).add("\n");
		//std::cerr << "erase handle " << *it << std::endl;
		_handle_to_signal_map.erase(*it);
	}
	_erased_handles.clear();
	logger.add("  _id_handles_map.erase(saftbus_id)\n");
	_id_handles_map.erase(saftbus_id);

	{
		saftbus::Timer f_time(_function_run_times["Connection::handle_disconnect_clean_fds_from_socket"]);
		clean_all_fds_from_socket(socket);
	}

	// "garbage collection" : remove all inactive objects
	std::vector<std::pair<Glib::ustring, Glib::ustring> > objects_to_be_erased;
	for (auto saftbus_index: _saftbus_indices) {
		Glib::ustring interface_name = saftbus_index.first;
		for (auto object_path: saftbus_index.second) {
			if (_saftbus_objects.find(object_path.second) == _saftbus_objects.end()) {
				objects_to_be_erased.push_back(std::make_pair(interface_name, object_path.first));
				std::cerr << "erasing " << interface_name << " " << object_path.first << std::endl;
			}
		}
	}
	for (auto erase: objects_to_be_erased) {
		_saftbus_indices[erase.first].erase(erase.second);
		if (_saftbus_indices[erase.first].size() == 0) {
			_saftbus_indices.erase(erase.first);
		}
	}




	logger.log();
	// if (_debug_level > 2) print_all_fds();
	// if (_debug_level > 2) std::cerr << "Connection::handle_disconnect() done" << std::endl;
}


double delta_t(struct timespec start, struct timespec stop) 
{
	return (1.0e6*stop.tv_sec   + 1.0e-3*stop.tv_nsec) 
         - (1.0e6*start.tv_sec   + 1.0e-3*start.tv_nsec);
}

// send a signal to all connected sockets
// I think I would be possible to filter the signals and send them only to sockets where they are actually expected
void Connection::emit_signal(const Glib::ustring& object_path, 
	                         const Glib::ustring& interface_name, 
	                         const Glib::ustring& signal_name, 
	                         const Glib::ustring& destination_bus_name, 
	                         const Glib::VariantContainerBase& parameters)
{
	saftbus::Timer f_time(_function_run_times["Connection::emit_signal"]);
	logger.newMsg(0).add("Connection::emit_signal(").add(object_path).add(",")
	                                                .add(interface_name).add(",")
	                                                .add(signal_name).add(",")
	                                                .add(destination_bus_name).add(",")
	                                                .add("parameters)\n");


    struct timespec start_time, packing_done_time, serialized_time;
    clock_gettime( CLOCK_REALTIME, &start_time);

	//if (_debug_level > 4) std::cerr << "Connection::emit_signal(" << object_path << "," << interface_name << "," << signal_name << "," << parameters.print() << ") called" << std::endl;
		// std::cerr << "   signals" << std::endl;
		// for(auto it = _owned_signals_signatures.begin() ; it != _owned_signals_signatures.end(); ++it) 
		// 		std::cerr << "     " << *it << std::endl;
	logger.add("    paramerters:\n");
	for (unsigned n = 0; n < parameters.get_n_children(); ++n)
	{
		Glib::VariantBase child;
		parameters.get_child(child, n);
		//std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
		logger.add("         ").add(n).add("   ").add(child.get_type_string()).add("   ").add(child.print()).add("\n");
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

	//if (_debug_level > 5) std::cerr << "signal message " << var_signal_msg.get_type_string() << " " << var_signal_msg.print() << std::endl;
	guint32 size = var_signal_msg.get_size();
	//if (_debug_level > 5) std::cerr << " size of signal is " << size << std::endl;
	logger.add("   size of serialized parameters: ").add(size).add("\n");
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
			//if (_debug_level > 2) std::cerr << "PropertiesChaned ----> derived_interface_name = " << derived_interface_name.get() << " " << object_path << std::endl;
			logger.add("     PropertiesChaned: ").add(derived_interface_name).add("\n");

			// directly send signal
			std::set<ProxyPipe> &setProxyPipe = _proxy_pipes[derived_interface_name.get()][object_path];
			for (auto i = setProxyPipe.begin(); i != setProxyPipe.end(); ++i) {
				try {
					bool error = false;
					logger.add("            writing to pipe: fd=").add(i->fd).add("    ");
					//if (_debug_level > 2) std::cerr << "sending signal through pipe " << i->fd << std::endl;
					if (write(i->fd, saftbus::SIGNAL) < 0) error = true;
					if (write(i->fd, size) < 0 ) error = true;
					if (write_all(i->fd, data_ptr, size) < 0 ) error = true;
					//if (_debug_level > 2) std::cerr << "done with signal through pipe" << std::endl;
					//if (error) std::cerr << "there was an error sending the Properties Changed signal" << std::endl;
					logger.add("  error=").add(error).add("\n");
				} catch(std::exception &e) {
					logger.add("\n   exception while writing to fd=").add(i->fd).add(" : ").add(e.what()).add("\n");
				}
			}

		} else {
			//if (_debug_level > 2) std::cerr << "Normal Signal ----> interface_name = " << interface_name << " " << object_path << std::endl;
			logger.add("     Normal signal: ").add(interface_name).add("\n");
			// directly send signal
			std::set<ProxyPipe> &setProxyPipe = _proxy_pipes[interface_name][object_path];
			for (auto i = setProxyPipe.begin(); i != setProxyPipe.end(); ++i) {
				try {
					bool error = false;
					logger.add("            writing to pipe: fd=").add(i->fd).add("    ");
					//if (_debug_level > 2) std::cerr << "sending signal through pipe " << i->fd  << std::endl;
					if (write(i->fd, saftbus::SIGNAL) < 0 ) error = true;
					if (write(i->fd, size) < 0 ) error = true;
					if (write_all(i->fd, data_ptr, size) < 0 ) error = true;
					//if (_debug_level > 2) std::cerr << "done with signal through pipe" << std::endl;
					//if (error) std::cerr << "there was an error sending the (real) signal" << std::endl;
					logger.add("  error=").add(error).add("\n");
				} catch(std::exception &e) {
					logger.add("\n   exception while writing to fd=").add(i->fd).add(" : ").add(e.what()).add("\n");
				}
			}
		}
	} catch(std::exception &e) {
		//std::cerr << "exception in Connection::dispatch(): " << e.what() << std::endl;
		logger.add("           exception in Connection::emit_signal() ").add(e.what()).log();
		//if (_debug_level > 5) print_all_fds();
	}

	// catch (...) {
	// 	//std::cerr << "caught a signal (Pipe full? Proxy without Glib::MainLoop?)" << std::endl;
	// 	logger.add("\n!!!       caught an Exception").add("\n");
	// }

	logger.log();

	//std::cerr << "signal packing time = " << delta_t(start_time, packing_done_time) << " us" << std::endl;
	//std::cerr << "serialization  time = " << delta_t(packing_done_time, serialized_time) << " us" << std::endl;
	//list_all_resources();
}



bool Connection::dispatch(Glib::IOCondition condition, Socket *socket) 
{
	logger.newMsg(0).add("Connection::dispatch(condition, ").add(socket->get_fd()).add(")\n");
	saftbus::Timer* f_time = new saftbus::Timer(_function_run_times["Connection::dispatch"]);
	int path_indicator = 0;
	try {
		static int cnt = 0;
		++cnt;
		//if (_debug_level > 2)  std::cerr << "Connection::dispatch() called one socket[" << socket_nr(socket) << "]" << std::endl;
		MessageTypeC2S type;
		int result = saftbus::read(socket->get_fd(), type);
		if (result == -1) {
			//if (_debug_level > 2) std::cerr << "client disconnected" << std::endl;
			logger.add("       client disconnected\n");
			handle_disconnect(socket);
		} else {
			switch(type)
			{
				case saftbus::SAFTBUS_CTL_HELLO:
				{
					path_indicator = 1;
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SAFTBUS_CTL_HELLO"]);
					//std::cerr << "got signal from saftbus-ctl tool" << std::endl;
					logger.add("   got ping from saftbus-ctl\n");
				}
				break;
				case saftbus::SAFTBUS_CTL_STATUS:
				{
					path_indicator = 2;
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SAFTBUS_CTL_STATUS"]);
					logger.add("   got stats request from saftbus-ctl\n");
					saftbus::write(socket->get_fd(), _saftbus_indices);
					std::vector<int> indices;
					for (auto it: _saftbus_objects) {
						indices.push_back(it.first);
					}
					saftbus::write(socket->get_fd(), indices);
					saftbus::write(socket->get_fd(), _signal_flight_times);
					saftbus::write(socket->get_fd(), _function_run_times);
				}
				break;
				case saftbus::SAFTBUS_CTL_GET_STATE:
				{

					//std::map<Glib::ustring, std::map<Glib::ustring, int> > _saftbus_indices; 
					saftbus::write(socket->get_fd(), _saftbus_indices);

					//std::map<int, std::shared_ptr<InterfaceVTable> > _saftbus_objects;
					std::vector<int> indices;
					for (auto it: _saftbus_objects) {
						indices.push_back(it.first);
					}
					saftbus::write(socket->get_fd(), indices);

					//int _saftbus_object_id_counter; // log saftbus object creation
					saftbus::write(socket->get_fd(), _saftbus_object_id_counter);
					//int _saftbus_signal_handle_counter; // log signal subscriptions
					saftbus::write(socket->get_fd(), _saftbus_signal_handle_counter);

					// TODO: use std::set instead of std::vector
					//std::vector<std::shared_ptr<Socket> > _sockets; 
					std::vector<int> sockets_active;
					for (auto socket: _sockets) {
						sockets_active.push_back(socket->get_active());
					}
					saftbus::write(socket->get_fd(), sockets_active);

					//int _client_id;


					// 	     // handle    // signal
					//std::map<guint, sigc::signal<void, const Glib::RefPtr<Connection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&> > _handle_to_signal_map;
					std::map<guint, int> handle_to_signal_map;
					for(auto handle: _handle_to_signal_map) {
						int num_slots = 0;
						for (auto slt: handle.second.slots()) {
							++num_slots;
						}
						handle_to_signal_map[handle.first] = num_slots;
					}
					saftbus::write(socket->get_fd(), handle_to_signal_map);



					//std::map<Glib::ustring, std::set<guint> > _id_handles_map;
					saftbus::write(socket->get_fd(), _id_handles_map);

					//std::set<guint> erased_handles;
					std::vector<guint> erased_handles;
					for(auto erased_handle: _erased_handles) {
						erased_handles.push_back(erased_handle);
					}
					saftbus::write(socket->get_fd(), erased_handles);



					// store the pipes that go directly to one or many Proxy objects
							// interface_name        // object path
					//std::map<Glib::ustring, std::map < Glib::ustring , std::set< ProxyPipe > > > _proxy_pipes;
					saftbus::write(socket->get_fd(), _proxy_pipes);

					//static int _saftbus_id_counter;
					saftbus::write(socket->get_fd(), _saftbus_id_counter);


				}
				break;
				case saftbus::SENDER_ID:
				{
					path_indicator = 3;
					//if (_debug_level > 2) std::cerr << "Connection::dispatch() SENDER_ID received" << std::endl;
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SENDER_ID"]);
					logger.add("     SENDER_ID received\n");
					Glib::ustring sender_id;
					//saftbus::read(socket->get_fd(), sender_id);
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
					path_indicator = 4;

				}
				break;
				case saftbus::SIGNAL_FLIGHT_TIME: 
				{
					path_indicator = 5;
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SIGNAL_FLIGHT_TIME"]);
					logger.add("     SIGNAL_FLIGHT_TIME received: ");
					double dt;
					read(socket->get_fd(), dt);
					int dt_us = dt;
					++_signal_flight_times[dt_us];
					logger.add(dt).add(" us\n");
					// if (dt > 600) {
					// 	std::cerr << "saftd: long signal flight time detected: " << dt << " us" << std::endl; 
					// }
					//std::cerr << "signal flight time reported: " << dt << " us" << std::endl;
				}
				break;
				case saftbus::SIGNAL_FD: 
				{
					path_indicator = 6;
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SIGNAL_FD"]);
					logger.add("     SIGNAL_FD received: ");
					//if (_debug_level > 2) std::cerr << "Connection::dispatch() received SIGNAL_FD" << std::endl;
					int fd = saftbus::recvfd(socket->get_fd());
					Glib::ustring name, object_path, interface_name;
					int proxy_id;
					read(socket->get_fd(), object_path);
					read(socket->get_fd(), interface_name);
					read(socket->get_fd(), proxy_id);
					//if (_debug_level > 2) std::cerr << "Connection::dispatch() got fd " << fd << " " << name << " " << object_path << " " << interface_name << " " << proxy_id << std::endl;
					logger.add("for object_path=").add(object_path)
					      .add(" interface_name=").add(interface_name)
					      .add(" proxy_id=").add(proxy_id)
					      .add(" fd=").add(fd).add("\n");

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
					path_indicator = 7;
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SIGNAL_REMOVE_FD"]);
					//if (_debug_level > 2) std::cerr << "Connection::dispatch() received SIGNAL_REMOVE_FD" << std::endl;
					//if (_debug_level > 2) print_all_fds();
					logger.add("           SIGNAL_REMOVE_FD received: ");
					Glib::ustring name, object_path, interface_name;
					int proxy_id;
					read(socket->get_fd(), object_path);
					read(socket->get_fd(), interface_name);
					read(socket->get_fd(), proxy_id);

					ProxyPipe pp;
					pp.id = proxy_id;
					auto pp_done = _proxy_pipes[interface_name][object_path].find(pp);
					close(pp_done->fd);
					logger.add("for object_path=").add(object_path)
					      .add(" interface_name=").add(interface_name)
					      .add(" proxy_id=").add(proxy_id)
					      .add(" fd=").add(pp_done->fd).add("\n");
					_proxy_pipes[interface_name][object_path].erase(pp_done);
					// if (_debug_level > 2) std::cerr << "Connection::dispatch() after erasing pipe fd" << std::endl;
					// if (_debug_level > 2) print_all_fds();
				}
				break;
				case saftbus::METHOD_CALL: 
				{
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_METHOD_CALL"]);
					logger.add("           METHOD_CALL received: ");
					//if (_debug_level > 2) std::cerr << "Connection::dispatch() METHOD_CALL received" << std::endl;
					guint32 size;
					saftbus::read(socket->get_fd(), size);
					//if (_debug_level > 5) std::cerr << "expecting message with size " << size << std::endl;
					logger.add(" message size=").add(size);
					std::vector<char> buffer(size);
					saftbus::read_all(socket->get_fd(), &buffer[0], size);
					Glib::Variant<std::vector<Glib::VariantBase> > payload;
					deserialize(payload, &buffer[0], buffer.size());
					Glib::Variant<Glib::ustring> object_path    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(0));
					Glib::Variant<Glib::ustring> sender         = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(1));
					Glib::Variant<Glib::ustring> interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(2));
					Glib::Variant<Glib::ustring> name           = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(3));
					Glib::VariantContainerBase parameters       = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>   (payload.get_child(4));
					logger.add(" sender=").add(sender.get())
					      .add(" name=").add(name.get())
					      .add(" object_path=").add(object_path.get())
					      .add(" interface_name=").add(interface_name.get())
					      .add(" \n");
					//if (_debug_level > 5) std::cerr << "sender = " << sender.get() <<  "    name = " << name.get() << "   object_path = " << object_path.get() <<  "    interface_name = " << interface_name.get() << std::endl;
					//if (_debug_level > 5) std::cerr << "parameters = " << parameters.print() << std::endl;
					if (interface_name.get() == "org.freedesktop.DBus.Properties") { // property get/set method call
						//if (_debug_level > 5) std::cerr << " interface for Set/Get property was called" << std::endl;
						logger.add("       Set/Get was called: ");

						Glib::Variant<Glib::ustring> derived_interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(0));
						Glib::Variant<Glib::ustring> property_name          = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(1));

						logger.add(" derived_interface_name=").add(derived_interface_name.get())
						      .add(" property_name=").add(property_name.get())
						      .add("\n");

						//if (_debug_level > 5) std::cerr << "property_name = " << property_name.get() << "   derived_interface_name = " << derived_interface_name.get() << std::endl;

						for (unsigned n = 0; n < parameters.get_n_children(); ++n)
						{
							Glib::VariantBase child;
							parameters.get_child(child, n);
							//if (_debug_level > 5) std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
						}					
						int index = _saftbus_indices[derived_interface_name.get()][object_path.get()];
						//if (_debug_level > 5) std::cerr << "found saftbus object at index " << index << std::endl;

						logger.add("    found saftbus object at index=").add(index).add("\n");
						
						if (name.get() == "Get") {
							path_indicator = 8;

							logger.add("      Get the property\n");

							saftbus::Timer f_time(_function_run_times["Connection::dispatch_METHOD_CALL_GetProperty"]);
							Glib::VariantBase result;
							_saftbus_objects[index]->get_property(result, saftbus::connection, sender.get(), object_path.get(), derived_interface_name.get(), property_name.get());
							//if (_debug_level > 2) std::cerr << "getting property " << result.get_type_string() << " " << result.print() << std::endl;
							//if (_debug_level > 5) std::cerr << "result is: " << result.get_type_string() << "   .value = " << result.print() << std::endl;

							std::vector<Glib::VariantBase> response;
							response.push_back(result);
							Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

							//if (_debug_level > 5) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
							size = var_response.get_size();
							//if (_debug_level > 5) std::cerr << " size of response is " << size << std::endl;
							const char *data_ptr = static_cast<const char*>(var_response.get_data());
							logger.add("         size of response -> serialized property: size=").add(size).add("\n");
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);
							logger.add("         property was sent\n");

						} else if (name.get() == "Set") {
							path_indicator = 9;
							logger.add("      Set the property\n");
							std::ostringstream function_name;
							function_name << "Connection::dispatch_METHOD_CALL_SetProperty_" << derived_interface_name.get() << "_" << property_name.get();
							saftbus::Timer f_time(_function_run_times[function_name.str().c_str()]);
							//if (_debug_level > 2) std::cerr << "setting property" << std::endl;
							Glib::Variant<Glib::VariantBase> par2 = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::VariantBase> >(parameters.get_child(2));
							//if (_debug_level > 5) std::cerr << "par2 = " << par2.get_type_string() << " "<< par2.print() << std::endl;
							// Glib::Variant<bool> vb = Glib::Variant<bool>::create(true);
							// std::cerr << "vb = " << vb.get_type_string() << " " << vb.print() << std::endl;

							//std::cerr << "value = " << value << std::endl;
							// if (_debug_level > 5) std::cerr << " value = " << value.get_type_string() << " " << value.print() << std::endl;
							bool result = _saftbus_objects[index]->set_property(saftbus::connection, sender.get(), object_path.get(), derived_interface_name.get(), property_name.get(), par2.get());

							std::vector<Glib::VariantBase> response;
							response.push_back(Glib::Variant<bool>::create(result));
							Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

							//if (_debug_level > 5) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
							size = var_response.get_size();
							//if (_debug_level > 5) std::cerr << " size of response is " << size << std::endl;
							const char *data_ptr = static_cast<const char*>(var_response.get_data());

							logger.add("         size of response -> empty resoponse: size=").add(size).add("\n");
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);
							logger.add("         response was sent\n");
						}

					}
					else // normal method calls 
					{
						path_indicator = 10;
						logger.add("         a normal method call\n");
						std::ostringstream function_name;
						function_name << "Connection::dispatch_METHOD_CALL_" << name.get();
						saftbus::Timer f_time(_function_run_times[function_name.str().c_str()]);

						//if (_debug_level > 2) std::cerr << "a real method call: " << std::endl;
						int index = _saftbus_indices[interface_name.get()][object_path.get()];
						//if (_debug_level > 5) std::cerr << "found saftbus object at index " << index << std::endl;
						Glib::RefPtr<MethodInvocation> method_invocation_rptr(new MethodInvocation);
						//if (_debug_level > 5) std::cerr << "saftbus::connection.get() " << static_cast<bool>(saftbus::connection) << std::endl;
						//if (_debug_level > 2) std::cerr << "doing the function call" << std::endl;
						logger.add("     doing the function call ...\n");
						_saftbus_objects[index]->method_call(saftbus::connection, sender.get(), object_path.get(), interface_name.get(), name.get(), parameters, method_invocation_rptr);
						logger.add("     ... done \n");
						//if (_debug_level > 2) std::cerr << "function call done, getting result" << std::endl;
						if (method_invocation_rptr->has_error()) {
							logger.add("     ... return an error \n");
							saftbus::write(socket->get_fd(), saftbus::METHOD_ERROR);
							saftbus::write(socket->get_fd(), method_invocation_rptr->get_return_error().type());
							saftbus::write(socket->get_fd(), method_invocation_rptr->get_return_error().what());
						} else {
							logger.add("     ... return a normal return value \n");
							Glib::VariantContainerBase result;
							result = method_invocation_rptr->get_return_value();
							//if (_debug_level > 2) std::cerr << "result is " << result.get_type_string() << " " << result.print() << std::endl;

							std::vector<Glib::VariantBase> response;
							response.push_back(result);
							Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

							//if (_debug_level > 5) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
							size = var_response.get_size();
							const char *data_ptr = static_cast<const char*>(var_response.get_data());
							logger.add("         size of response -> method resoponse: size=").add(size).add("\n");
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);
							logger.add("         response was sent\n");
						}
					}
				}
				break;

				default:
					//if (_debug_level > 5) std::cerr << "Connection::dispatch() unknown message type: " << type << std::endl;
					logger.add("      unknown message type\n");
					logger.log();
					return false;
				break;				
			}

			logger.log();


			double dt = f_time->delta_t();
			delete f_time;
			if (dt > 1e5) {
				std::cerr << dt << " more than 100ms for dispatch. " << path_indicator << std::endl;
			}
			if (dt > 5e5) {
				std::cerr << dt << " more than 500ms for dispatch. " << path_indicator << std::endl;
			}

			return true;
		}
	} catch(std::exception &e) {
		//std::cerr << "exception in Connection::dispatch(): " << e.what() << std::endl;
		logger.add("           exception in Connection::dispatch()").add(e.what()).log();
		//if (_debug_level > 5) print_all_fds();
		handle_disconnect(socket);
		//if (_debug_level > 5) print_all_fds();
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
	saftbus::Timer f_time(_function_run_times["Connection::clean_all_fds_from_socket"]);
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
