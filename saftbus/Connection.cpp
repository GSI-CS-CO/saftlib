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
	, _create_signal_flight_time_statistics(false)
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
	logger.newMsg(1).add("Connection::~Connection()\n").log();
}

// Add an object to the set of saftbus controlled objects. Each registered object has a unique ID.
// The ID is returned by this function to Saftlib and is used later to unregister the object.
// This ID is internal to the saftd. Saftbus proxies never use this ID.
// Saftbus proxies address saftbus objects using object_path and interface_name.
// object_path and interface_name link to the vtable given to this function (a table with functions for 
// "method_call" "set_property" and "get_property"). The interface_name is extracted from the 
// interface_info object. 
unsigned Connection::register_object (const std::string& object_path, 
								   const std::shared_ptr< InterfaceInfo >& interface_info, 
								   const InterfaceVTable& vtable)
{
	++_saftbus_object_id_counter;
	logger.newMsg(1).add("Connection::register_object(").add(object_path).add(")\n");
	unsigned registration_id = _saftbus_object_id_counter;
	_saftbus_objects[_saftbus_object_id_counter] = std::shared_ptr<InterfaceVTable>(new InterfaceVTable(vtable));
	std::string interface_name = interface_info->get_interface_name();
	logger.add(" interface_name:").add(interface_name).add(" -> id:").add(registration_id).log();
	_saftbus_indices[interface_name][object_path] = registration_id;
	return registration_id;
}
bool Connection::unregister_object (unsigned registration_id)
{
	logger.newMsg(0).add("Connection::unregister_object(").add(registration_id).add(")\n").log();

	_saftbus_objects.erase(registration_id);
	return true;
	//list_all_resources();
}


// subscribe to local signal (local inside the saftbus process)
// all registered signals will be redirected to the saftbus_objects. These are of type Owned. 
// slot is the function that will be the disconnection handler of the saftlib object
// sender, interface_name, member, and object_path are not used here
// arg0 is the saftbus_id of the object that is also known to the proxies
unsigned Connection::signal_subscribe(const SlotSignal& slot,
								   const std::string& sender,
								   const std::string& interface_name,
								   const std::string& member,
								   const std::string& object_path,
								   const std::string& arg0)
{
	try {
		saftbus::Timer f_time(_function_run_times["Connection::signal_subscribe"]);
		logger.newMsg(1).add("Connection::signal_subscribe(")
		                .add(sender).add(",")
		                .add(interface_name).add(",")
		                .add(member).add(",")
		                .add(object_path).add(",")
		                .add(arg0)
		                .add(")\n");

		// save the slot (the function object) under a unique subscription_id.
		// the id is created by incrementing a counter
		int subscription_id = ++_saftbus_signal_handle_counter;
		_handle_to_signal_map[subscription_id].connect(slot);

		_id_handles_map[arg0].insert(subscription_id);

		for(auto it: _id_handles_map) {
			for (auto i: it.second) {
				logger.add("     _id_handles_map[").add(it.first).add("]: ").add(i).add("\n");
			}
		}
		logger.add("            return signal handle ").add(subscription_id).log();
		return subscription_id;
	} catch(std::exception &e) {
		std::cerr << "Connection::signal_subscribe() exception : " << e.what() << std::endl;
	}
	return 0xffffffff;
}

void Connection::signal_unsubscribe(unsigned subscription_id) 
{
	if (subscription_id == 0xffffffff) {
		std::cerr << "Connection::signal_unsubscribe(-1) is invalid" << std::endl;
		return;
	}
	try {
		logger.newMsg(1).add("signal_unsubscribe(").add(subscription_id).add(")\n");
		saftbus::Timer f_time(_function_run_times["Connection::signal_unsubscribe"]);
		for(auto it: _handle_to_signal_map) {
			logger.add("       _owned_signals[").add(it.first).add("]\n");
		}
		logger.log();
		_erased_handles.insert(subscription_id);
	} catch (std::exception &e) {
		std::cerr << "Connection::signal_unsubscribe() exception : " << e.what() << std::endl;
	}
}

void Connection::handle_disconnect(Socket *socket)
{
	try {
		saftbus::Timer f_time(_function_run_times["Connection::handle_disconnect"]);
		logger.newMsg(1).add("Connection::handle_disconnect()\n");
		Serial dummy_arg;
		std::string& saftbus_id = socket->saftbus_id();

		_socket_owner.erase(socket_nr(socket));

		// make the socket available for new connection requests
		socket->close_connection();
		socket->wait_for_client();

		if (_id_handles_map.find(saftbus_id) != _id_handles_map.end()) { // we have to emit the quit signals
			for(auto it = _id_handles_map[saftbus_id].begin(); it != _id_handles_map[saftbus_id].end(); ++it) {
				logger.add("      _handle_to_signal_map[").add(*it).add("]\n");
			}
		}

		logger.add("   collect unsubscription handles:\n");


		if (_id_handles_map.find(saftbus_id) != _id_handles_map.end()) { 
			// We have an entry with this saftbus_id, so we have to emit quit signals.
			// This will cause the saftlib device to remove itself and free all resources.
			for (auto id_handles : _id_handles_map[saftbus_id]) {
				// The emitted signal will cause the signal_unsubscribe function (see above) to be called.
				// A signal should only be sent if (subscription_id) is not in the list of _erased_handles. 
				// I.e. don't emit signals to already erased saftlib services.
				if (_erased_handles.find(id_handles) == _erased_handles.end()) {
					logger.add("   sending signal to clean up: ").add(id_handles).add("\n").log();
					_handle_to_signal_map[id_handles].emit(saftbus::connection, "", "", "", "" , dummy_arg);
				}
			}
		}


		logger.newMsg(1).add("   delete unsubscription handles\n");
		// go through the list of subscribtion_ids  that were marked as erased and actually erase them
		//for (auto it = _erased_handles.begin(); it != _erased_handles.end(); ++it) {
		for (auto erased_handle: _erased_handles) {
			_handle_to_signal_map.erase(erased_handle);
		}
		_erased_handles.clear();
		logger.add("  _id_handles_map.erase(saftbus_id)\n");
		_id_handles_map.erase(saftbus_id);


		// "garbage collection" : remove all inactive objects
		std::vector<std::pair<std::string, std::string> > objects_to_be_erased;
		for (auto saftbus_index: _saftbus_indices) {
			std::string interface_name = saftbus_index.first;
			for (auto object_path: saftbus_index.second) {
				if (_saftbus_objects.find(object_path.second) == _saftbus_objects.end()) {
					objects_to_be_erased.push_back(std::make_pair(interface_name, object_path.first));
					//std::cerr << "erasing " << interface_name << " " << object_path.first << std::endl;
				}
			}
		}
		for (auto erase: objects_to_be_erased) {
			_saftbus_indices[erase.first].erase(erase.second);
			if (_saftbus_indices[erase.first].size() == 0) {
				_saftbus_indices.erase(erase.first);
			}
		}

		// remove all signal pipe file descriptors that were registered for this socket
		{
			saftbus::Timer f_time(_function_run_times["Connection::handle_disconnect_clean_fds_from_socket"]);
			clean_all_fds_from_socket(socket);
		}


		logger.log();
	} catch (std::exception &e) {
		std::cerr << "Connection::handle_disconnect() exception : " << e.what() << std::endl;
	}
}


// send a signal to all connected sockets
// I think I would be possible to filter the signals and send them only to sockets where they are actually expected
void Connection::emit_signal(const std::string& object_path, 
	                         const std::string& interface_name, 
	                         const std::string& signal_name, 
	                         const std::string& destination_bus_name, 
	                         const Serial&      parameters)
{
	try {
		// get the time when saftbus started processing the signal
	    struct timespec start_time;
	    clock_gettime( CLOCK_REALTIME, &start_time);

		saftbus::Timer f_time(_function_run_times["Connection::emit_signal"]);
		logger.newMsg(0).add("Connection::emit_signal(").add(object_path).add(",")
		                                                .add(interface_name).add(",")
		                                                .add(signal_name).add(",")
		                                                .add(destination_bus_name).add(",")
		                                                .add("parameters)\n");

		// use saftbus::Serial to serialize the signal data
		Serial signal_msg;
		signal_msg.put(object_path);
		signal_msg.put(interface_name);
		signal_msg.put(signal_name);
		signal_msg.put(start_time.tv_sec);
		signal_msg.put(start_time.tv_nsec);
		signal_msg.put(_create_signal_flight_time_statistics);
		signal_msg.put(parameters);
		unsigned    size     = signal_msg.get_size();
		const char* data_ptr = signal_msg.get_data();

		try {  
			// it may happen that the Proxy that is supposed to read the signal
			// does not have a running MainLoop. In this case the pipe will become full
			// at some point and the write function will throw. We have to catch that
			// to prevent the saft daemon from crashing
			std::set<ProxyPipe> &setProxyPipe = _proxy_pipes[interface_name][object_path];
			for (auto i = setProxyPipe.begin(); i != setProxyPipe.end(); ++i) {
				try {
					logger.add("            writing to pipe: fd=").add(i->fd).add("    ");
					saftbus::write(i->fd, saftbus::SIGNAL);
					saftbus::write(i->fd, size);
					saftbus::write_all(i->fd, data_ptr, size);
				} catch(std::exception &e) {
					logger.add("\n   exception while writing to fd=").add(i->fd).add(" : ").add(e.what()).add("\n");
					logger.add("    setProxyPipe.size() = ").add(setProxyPipe.size()).add("\n");
				}
			}
			//std::cerr << "Connection::emit_signal(" << object_path << ", " << interface_name << ", " << signal_name << ") done" << std::endl;
		} catch(std::exception &e) {
			// just continue, if something fails with this proxy, we still have other ones to care about
			logger.add("           exception in Connection::emit_signal() ").add(e.what()).log();
		}
		logger.log();
	} catch (std::exception &e) {
		// just continue, if something fails with this proxy, we still have other ones to care about
		std::cerr << "Connection::emit_signal() exception : " << e.what() << std::endl;
	}
}



bool Connection::dispatch(Slib::IOCondition condition, Socket *socket) 
{
	// handle a request coming from one of the sockets
	try {
		logger.newMsg(0).add("Connection::dispatch(condition, ").add(socket->get_fd()).add(")\n");
		//saftbus::Timer* f_time = new saftbus::Timer(_function_run_times["Connection::dispatch"]);

		// determine the request type
		MessageTypeC2S type;
		int result = saftbus::read(socket->get_fd(), type);
		if (result == -1) {
			logger.add("       client disconnected\n");
			handle_disconnect(socket);
		} else {
			switch(type)
			{
				case saftbus::SAFTBUS_CTL_ENABLE_LOGGING:
				{
					logger.enable();
					logger.newMsg(0).add("saftbus logging enabled").log();
				}
				break;
				case saftbus::SAFTBUS_CTL_DISABLE_LOGGING:
				{
					logger.newMsg(0).add("saftbus logging disabled").log();
					logger.disable();
				}
				break;
				case saftbus::SAFTBUS_CTL_ENABLE_STATS:
				{
					_create_signal_flight_time_statistics = true;
					std::cerr << "_signal_flight_times_stats=" << _create_signal_flight_time_statistics << std::endl;
				}
				break;
				case saftbus::SAFTBUS_CTL_DISABLE_STATS:
				{
					_create_signal_flight_time_statistics = false;
					std::cerr << "_signal_flight_times_stats=" << _create_signal_flight_time_statistics << std::endl;
				}
				break;
				case saftbus::SAFTBUS_CTL_GET_STATS:
				{
					saftbus::write(socket->get_fd(), _signal_flight_times);
					std::cerr << "_signal_flight_times_stats=" << _create_signal_flight_time_statistics << std::endl;
				}
				break;
				case saftbus::SAFTBUS_CTL_HELLO:
				{
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SAFTBUS_CTL_HELLO"]);
					logger.add("   got ping from saftbus-ctl\n");
				}
				break;
				case saftbus::SAFTBUS_CTL_STATUS:
				{
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
					// Send all mutable state variables of the Connection object.
					// This is supposed to be used by the saftbus-ctl tool.

					//std::map<std::string, std::map<std::string, int> > _saftbus_indices; 
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

					//std::vector<std::shared_ptr<Socket> > _sockets; 
					std::vector<int> sockets_active;
					for (auto socket: _sockets) {
						sockets_active.push_back(socket->get_active());
					}
					saftbus::write(socket->get_fd(), sockets_active);

					saftbus::write(socket->get_fd(), _socket_owner);

					// 	     // handle    // signal
					//std::map<unsigned, sigc::signal<void, const std::shared_ptr<Connection>&, const std::string&, const std::string&, const std::string&, const std::string&, const Glib::VariantContainerBase&> > _handle_to_signal_map;
					std::map<unsigned, int> handle_to_signal_map;
					for(auto handle: _handle_to_signal_map) {
						int num_slots = 0;
						for (auto slt: handle.second.slots()) {
							++num_slots;
						}
						handle_to_signal_map[handle.first] = num_slots;
					}
					saftbus::write(socket->get_fd(), handle_to_signal_map);

					//std::map<std::string, std::set<unsigned> > _id_handles_map;
					saftbus::write(socket->get_fd(), _id_handles_map);

					//std::set<unsigned> erased_handles;
					std::vector<unsigned> erased_handles;
					for(auto erased_handle: _erased_handles) {
						erased_handles.push_back(erased_handle);
					}
					saftbus::write(socket->get_fd(), erased_handles);

					// store the pipes that go directly to one or many Proxy objects
							// interface_name        // object path
					//std::map<std::string, std::map < std::string , std::set< ProxyPipe > > > _proxy_pipes;
					saftbus::write(socket->get_fd(), _proxy_pipes);

					//static int _saftbus_id_counter;
					saftbus::write(socket->get_fd(), _saftbus_id_counter);

					// list owners
					// map object path to owner
					std::map<std::string, std::string> owners;
					auto owned_object_paths = _saftbus_indices["de.gsi.saftlib.Owned"];
					for (auto object_path_index: owned_object_paths) {
						std::string obj_path = object_path_index.first;
						int idx              = object_path_index.second;
						//std::cerr << obj_path << " " << idx << std::endl;
						Serial result;
						// const std::string &sender = _saftbus_objects[idx]->getSender();
						// const std::string &op     = _saftbus_objects[idx]->getObjectPath();
						_saftbus_objects[idx]->get_property(result, saftbus::connection, "", "", "", "Owner");
						std::string owner;
						result.get_init();
						result.get(owner);
						owners[obj_path] = owner;
					}
					saftbus::write(socket->get_fd(), owners);

				}
				break;
				case saftbus::SENDER_ID:
				{
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SENDER_ID"]);
					logger.add("     SENDER_ID received\n");
					std::string sender_id;
					// generate a new id value (increasing numbers)
					++_saftbus_id_counter; 
					std::ostringstream id_out;
					id_out << ":" << _saftbus_id_counter;
					socket->saftbus_id() = id_out.str();
					//std::cerr << "socket_nr " << socket_nr(socket) << "   id " << id_out.str() << std::endl;
					_socket_owner[socket_nr(socket)] = id_out.str();
					// send the id to the ProxyConnection
					saftbus::write(socket->get_fd(), socket->saftbus_id());
				}
				case saftbus::REGISTER_CLIENT: 
				{

				}
				break;
				case saftbus::SIGNAL_FLIGHT_TIME: 
				{
					// after receiving a signal, the ProxyConnection will report
					// the signal flight time so that we can make a histogram
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SIGNAL_FLIGHT_TIME"]);
					logger.add("     SIGNAL_FLIGHT_TIME received: ");
					double dt;
					saftbus::read(socket->get_fd(), dt);
					int dt_us = dt;
					++_signal_flight_times[dt_us];
					logger.add(dt).add(" us\n");
				}
				break;
				case saftbus::GET_SAFTBUS_INDEX: 
				{
					saftbus::Timer f_time(_function_run_times["Connection::GET_SAFTBUS_INDEX"]);
					logger.add("     GET_SAFTBUS_INDEX received: ");
					std::string object_path, interface_name;
					saftbus::read(socket->get_fd(), object_path);
					saftbus::read(socket->get_fd(), interface_name);
					saftbus::write(socket->get_fd(), _saftbus_indices[interface_name][object_path]);
				}
				break;
				case saftbus::SIGNAL_FD: 
				{
					// each Proxy constructor will send the reading end of a pipe
					// to us as a fast path for signals
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SIGNAL_FD"]);
					logger.add("     SIGNAL_FD received: ");
					int fd = saftbus::recvfd(socket->get_fd());
					std::string name, object_path, interface_name;
					int proxy_id;
					saftbus::read(socket->get_fd(), object_path);
					saftbus::read(socket->get_fd(), interface_name);
					saftbus::read(socket->get_fd(), proxy_id);

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
					char ch = 'x';
					//std::cerr << "writing ping: " << ch << std::endl;
					saftbus::write(fd, ch);
				}
				break;
				case saftbus::SIGNAL_REMOVE_FD: 
				{
					// when a Proxy is destroyed it will send a signal remove request
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SIGNAL_REMOVE_FD"]);

					logger.add("           SIGNAL_REMOVE_FD received: ");
					std::string name, object_path, interface_name;
					int proxy_id;
					saftbus::read(socket->get_fd(), object_path);
					saftbus::read(socket->get_fd(), interface_name);
					saftbus::read(socket->get_fd(), proxy_id);

					ProxyPipe pp;
					pp.id = proxy_id;
					auto pp_done = _proxy_pipes[interface_name][object_path].find(pp);
					close(pp_done->fd);
					logger.add("for object_path=").add(object_path)
					      .add(" interface_name=").add(interface_name)
					      .add(" proxy_id=").add(proxy_id)
					      .add(" fd=").add(pp_done->fd).add("\n");
					_proxy_pipes[interface_name][object_path].erase(pp_done);

					proxy_pipe_garbage_collection();

				}
				break;
				case saftbus::METHOD_CALL: 
				{
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_METHOD_CALL"]);
					logger.add("           METHOD_CALL received: ");
					
					// read the size of serialized data
					uint32_t size;
					saftbus::read(socket->get_fd(), size);
					logger.add(" message size=").add(size);

					// read the serialized data
					Serial payload;
					payload.data().resize(size);
					saftbus::read_all(socket->get_fd(), &payload.data()[0], size);

					// saftbus::Serial to get content
					std::string object_path;
					std::string sender;
					std::string interface_name;
					std::string name;
					Serial parameters;

					payload.get_init();
					payload.get(object_path);
					payload.get(sender);
					payload.get(interface_name);
					payload.get(name);
					payload.get(parameters);

					logger.add(" sender=").add(sender)
					      .add(" name=").add(name)
					      .add(" object_path=").add(object_path)
					      .add(" interface_name=").add(interface_name)
					      .add(" \n");

					if (interface_name == "org.freedesktop.DBus.Properties") { // property get/set method call

						logger.add("       Set/Get was called: ");
						std::string derived_interface_name;
						std::string property_name;
						parameters.get_init();
						parameters.get(derived_interface_name);
						parameters.get(property_name);
						logger.add(" derived_interface_name=").add(derived_interface_name)
						      .add(" property_name=").add(property_name)
						      .add("\n");
				
						// saftbus object lookup
						int index = _saftbus_indices[derived_interface_name][object_path];
						if (index == 0) {// == not found
							// this causes an exception on the proxy side
							std::string what("unknown object path: ");
							what.append(object_path);
							saftbus::write(socket->get_fd(), saftbus::METHOD_ERROR);
							saftbus::write(socket->get_fd(), saftbus::Error::FAILED);
							saftbus::write(socket->get_fd(), what);
							logger.add(what);
							logger.log();
							break;
						}
						logger.add("    found saftbus object at index=").add(index).add("\n");
						if (name == "Get") {
							logger.add("      Get the property\n");
							saftbus::Timer f_time(_function_run_times["Connection::dispatch_METHOD_CALL_GetProperty"]);

							// call auto-generated get_porperty handler using the vtable
							Serial result;
							_saftbus_objects[index]->get_property(result, saftbus::connection, sender, object_path, derived_interface_name, property_name);

							// prepare response data
							// serialize response data
							size = result.get_size();
							const char *data_ptr = static_cast<const char*>(result.get_data());
							// write data to socket
							logger.add("         size of response (serialized property): ").add(size).add("\n");
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);
							logger.add("         property was sent\n");

						} else if (name == "Set") {
							logger.add("      Set the property\n");
							std::ostringstream function_name;
							function_name << "Connection::dispatch_METHOD_CALL_SetProperty_" << derived_interface_name << "_" << property_name;
							saftbus::Timer f_time(_function_run_times[function_name.str().c_str()]);
							
							// set the value using the set_property handler from auto-generated saftlib code
							Serial par2;
							parameters.get(par2);
							bool result = _saftbus_objects[index]->set_property(saftbus::connection, sender, object_path, derived_interface_name, property_name, par2);

							// even the set property has a response ...
							Serial response;
							response.put(result);

							// serialize the data
							size = response.get_size();
							const char *data_ptr = static_cast<const char*>(response.get_data());

							// send the data 
							logger.add("         size of response (empty resoponse): ").add(size).add("\n");
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);
							logger.add("         response was sent\n");
						}

					}
					else // normal method calls 
					{
						logger.add("         regular method call\n");
						std::ostringstream function_name;
						function_name << "Connection::dispatch_METHOD_CALL_" << name;
						saftbus::Timer f_time(_function_run_times[function_name.str().c_str()]);

						// saftbus object lookup
						int index = _saftbus_indices[interface_name][object_path];
						if (index == 0) {// == not found
							// this causes an exception on the proxy side
							std::string what("unknown object path: ");
							what.append(object_path);
							saftbus::write(socket->get_fd(), saftbus::METHOD_ERROR);
							saftbus::write(socket->get_fd(), saftbus::Error::FAILED);
							saftbus::write(socket->get_fd(), what);
							logger.add(what);
							logger.log();
							break;
						}

						std::shared_ptr<MethodInvocation> method_invocation_rptr(new MethodInvocation);

						logger.add("         calling the driver function ...\n");
						_saftbus_objects[index]->method_call(saftbus::connection, sender, object_path, interface_name, name, parameters, method_invocation_rptr);
						logger.add("         ... done \n");

						if (method_invocation_rptr->has_error()) {
							logger.add("         return an error \n");
							saftbus::write(socket->get_fd(), saftbus::METHOD_ERROR);
							saftbus::write(socket->get_fd(), method_invocation_rptr->get_return_error().type());
							saftbus::write(socket->get_fd(), method_invocation_rptr->get_return_error().what());
						} else {
							logger.add("         regular return \n");

							// get the result and pack it in a way that 
							//   can be digested by the auto-generated saftlib code
 							Serial &result = method_invocation_rptr->get_return_value();

							// serialize
							size = result.get_size();
							const char *data_ptr = static_cast<const char*>(result.get_data());
							logger.add("         size of response (return value): ").add(size).add("\n");

							//send 
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							if (size > 0) {
								saftbus::write_all(socket->get_fd(), data_ptr, size);
							}
							logger.add("         response was sent\n");
						}
					}
				}
				break;
				default:
					logger.add("      unknown message type\n");
					logger.log();
					handle_disconnect(socket);
					return false;
				break;				
			}
			logger.log();

			return true;
		}
	} catch(std::exception &e) {
		//std::cerr << "exception in Connection::dispatch(): " << e.what() << std::endl;
		logger.add("           exception in Connection::dispatch()").add(e.what()).log();
		//if (_debug_level > 5) print_all_fds();
		handle_disconnect(socket);
		//if (_debug_level > 5) print_all_fds();
	} catch (saftbus::Error &e) {
		std::cerr << "saftbus::Error " << e.what() << std::endl;
	}
	return false;
}

void Connection::clean_all_fds_from_socket(Socket *socket)
{
	try {
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
		// more complete garbage collection
		proxy_pipe_garbage_collection();
	} catch( std::exception & e) {
		std::cerr << "Connection::clean_all_fds_from_socket() exception : " << e.what() << std::endl;
	}

}

int Connection::socket_nr(Socket *socket)
{
	for (uint32_t i = 0; i < _sockets.size(); ++i) {
		if (_sockets[i].get() == socket)
			return i;
	}
	return -1;
}

void Connection::proxy_pipe_garbage_collection()
{
	std::map<std::string, std::map < std::string , std::set< ProxyPipe > > > new_proxy_pipes;
	for (auto iter1: _proxy_pipes) {
		for (auto iter2: iter1.second) {
			for(auto pp: iter2.second) {
				new_proxy_pipes[iter1.first][iter2.first].insert(pp);
			}
		}
	}
	_proxy_pipes = new_proxy_pipes;
}


}
