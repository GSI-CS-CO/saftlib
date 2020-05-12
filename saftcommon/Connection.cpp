/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include "Connection.h"
#include "core.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace saftbus
{

int Connection::_saftbus_id_counter = 0;

Connection::Connection(const std::string& base_name)
	: _saftbus_object_id_counter(1)
	, _saftbus_signal_handle_counter(1)
	, logger("/tmp/saftbus_connection.log")
	, _create_signal_flight_time_statistics(false)
{
	char *socketname_env = getenv(saftbus_socket_environment_variable_name);
	std::string socketname = base_name;
	if (socketname_env != nullptr) {
		socketname = socketname_env;
	}
	if (socketname[0] != '/') {
		throw std::runtime_error("saftbus socket is not an absolute pathname");
	}
	_listen_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	std::string err_msg = "Connection constructor errror: ";
	if (_listen_fd <= 0) {
		err_msg.append(strerror(errno));
		throw std::runtime_error(err_msg);
	}
	std::string dirname = socketname.substr(0,socketname.find_last_of('/'));
	std::ostringstream command;
	command << "mkdir -p " << dirname;
	int result = system(command.str().c_str());
	if (result == -1) {
		std::ostringstream msg;
		msg << "system call \"" << command.str() << "\" failed: " << strerror(errno);
		throw std::runtime_error(msg.str().c_str());
	}
	int unlink_result = unlink(socketname.c_str());
	std::ostringstream unlink_error;
	if (unlink_result != 0) {
		unlink_error << "could not unlink socket file " << socketname << ": " << strerror(errno);
	}
	_listen_sockaddr_un.sun_family = AF_LOCAL;
	strcpy(_listen_sockaddr_un.sun_path, socketname.c_str());
	int bind_result = bind(_listen_fd, (struct sockaddr*)&_listen_sockaddr_un, sizeof(_listen_sockaddr_un));
	if (bind_result != 0) {
		std::ostringstream msg;
		msg << "could not bind to socket: " << strerror(errno);
		throw std::runtime_error(msg.str().c_str());
	}
	chmod(base_name.c_str(), S_IRUSR | S_IWUSR | 
		                     S_IRGRP | S_IWGRP | 
		                     S_IROTH | S_IWOTH );
	Slib::signal_io().connect(sigc::mem_fun(*this, &Connection::accept_client), _listen_fd, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_LOW);
}

bool Connection::accept_client(Slib::IOCondition condition) 
{
	int client_fd = recvfd(_listen_fd);
	_sockets.insert(client_fd);
	Slib::signal_io().connect(sigc::bind(sigc::mem_fun(this, &Connection::dispatch), client_fd), client_fd, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_HIGH);
	return true; // continue listening
}

Connection::~Connection()
{
	close(_listen_fd);
	logger.newMsg(1).add("Connection::~Connection()\n").log();
}

// Add an object to the set of saftbus controlled objects. 
// Each registered object has a unique ID.
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
	//logger.newMsg(1).add("Connection::register_object(").add(object_path).add(")\n");
	unsigned saftbus_object_id = _saftbus_object_id_counter;
	_saftbus_objects[_saftbus_object_id_counter] = std::shared_ptr<InterfaceVTable>(new InterfaceVTable(vtable));
	std::string interface_name = interface_info->get_interface_name();
	//logger.add(" interface_name:").add(interface_name).add(" -> id:").add(saftbus_object_id).log();

	// fill the lookup table to map interface_name and object path to saftbus_object_id
	_saftbus_object_ids[interface_name][object_path] = saftbus_object_id;
	return saftbus_object_id;
}
bool Connection::unregister_object (unsigned saftbus_object_id)
{
	logger.newMsg(0).add("Connection::unregister_object(").add(saftbus_object_id).add(")\n").log();
	_saftbus_objects.erase(saftbus_object_id);
	return true;
	//list_all_resources();
}


// subscribe to local signal (local inside the saftbus process)
// all registered signals will be redirected to the saftbus_objects. These are of type Owned. 
// slot is the function that will be the disconnection handler of the saftlib object
// sender, interface_name, member, and object_path are not used here
// saftbus_id is coming from the client process, i.e. this method makes only sense if called in the 
// context of a remote function call. In the case of saftlib, that is a call to the Owned::Own() function
// in file drivers/Owned.cpp .
unsigned Connection::signal_subscribe(const SlotSignal& slot,
								   const std::string& sender,
								   const std::string& interface_name,
								   const std::string& member,
								   const std::string& object_path,
								   const std::string& saftbus_id)
{
	try {
		saftbus::Timer f_time(_function_run_times["Connection::signal_subscribe"]);
		logger.newMsg(1).add("Connection::signal_subscribe(")
		                .add(sender).add(",")
		                .add(interface_name).add(",")
		                .add(member).add(",")
		                .add(object_path).add(",")
		                .add(saftbus_id)
		                .add(")\n");

		// save the slot (the function object) under a unique subscription_id.
		// the id is created by incrementing a counter
		int subscription_id = ++_saftbus_signal_handle_counter;
		_handle_to_signal_map[subscription_id].connect(slot);

		_id_handles_map[saftbus_id].insert(subscription_id);

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

void Connection::handle_disconnect(int client_fd)
{
	try {
		saftbus::Timer f_time(_function_run_times["Connection::handle_disconnect"]);
		logger.newMsg(1).add("Connection::handle_disconnect()\n");
		Serial dummy_arg;
		std::string& saftbus_id = _socket_owner[client_fd];

		close(client_fd);

		logger.add("   collect unsubscription handles:\n");
		if (_id_handles_map.find(saftbus_id) != _id_handles_map.end()) { 
			// We have a signal subscription with this saftbus_id, so we have to emit signals.
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
		for (auto saftbus_index: _saftbus_object_ids) {
			std::string interface_name = saftbus_index.first;
			for (auto object_path: saftbus_index.second) {
				if (_saftbus_objects.find(object_path.second) == _saftbus_objects.end()) {
					objects_to_be_erased.push_back(std::make_pair(interface_name, object_path.first));
					//std::cerr << "erasing " << interface_name << " " << object_path.first << std::endl;
				}
			}
		}
		for (auto erase: objects_to_be_erased) {
			std::string &interface_name = erase.first;
			std::string &object_path    = erase.second;
			_saftbus_object_ids[interface_name].erase(object_path);
			if (_saftbus_object_ids[interface_name].size() == 0) {
				_saftbus_object_ids.erase(interface_name);
			}
			for (auto pp: _signal_fds[interface_name][object_path]) {
				close(pp.fd);
				pp.con.disconnect();
			}
			_signal_fds[interface_name].erase(object_path);
		}

		// remove socket file descriptor
		_socket_owner.erase(client_fd);
		_sockets.erase(client_fd);

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

	    //std::cerr << "emit signal " << object_path << " " << interface_name << " " << signal_name << std::endl;

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
			std::set<SignalFD> &setSignalFD = _signal_fds[interface_name][object_path];
			for (auto i = setSignalFD.begin(); i != setSignalFD.end(); ++i) {
				try {
					logger.add("            writing to pipe: fd=").add(i->fd).add("    ");
					saftbus::write(i->fd, saftbus::SIGNAL);
					saftbus::write(i->fd, size);
					saftbus::write_all(i->fd, data_ptr, size);
				} catch(std::exception &e) {
					logger.add("\n   exception while writing to fd=").add(i->fd).add(" : ").add(e.what()).add("\n");
					logger.add("    setSignalFD.size() = ").add(setSignalFD.size()).add("\n");
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

bool Connection::proxy_pipe_closed(Slib::IOCondition condition, std::string interface_name, std::string object_path, SignalFD pp) 
{
	//std::cerr << "proxy pipe closed (HUP received) for interface " << interface_name << "    on object path " << object_path << std::endl;
	close(pp.fd);
	std::set<SignalFD> &proxy_pipes = _signal_fds[interface_name][object_path];
	proxy_pipes.erase(pp);
	if (_signal_fds.size() == 0) {
		_signal_fds[interface_name].erase(object_path);
		_signal_fds.erase(interface_name);
	}
	return false;
}

bool Connection::dispatch(Slib::IOCondition condition, int client_fd) 
{
	// handle a request coming from one of the sockets
	try {
		logger.newMsg(0).add("Connection::dispatch(condition, ").add(client_fd).add(")\n");

		if (condition & Slib::IO_HUP) {
			handle_disconnect(client_fd);
			return false;
		}
		// determine the request type
		MessageTypeC2S type;
		int result = saftbus::read(client_fd, type);
		if (result == -1) {
			logger.add("       client disconnected\n");
			handle_disconnect(client_fd);
			return false;
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
					saftbus::write(client_fd, _signal_flight_times);
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
					saftbus::write(client_fd, _saftbus_object_ids);
					std::vector<int> indices;
					for (auto it: _saftbus_objects) {
						indices.push_back(it.first);
					}
					saftbus::write(client_fd, indices);
					saftbus::write(client_fd, _signal_flight_times);
					saftbus::write(client_fd, _function_run_times);
				}
				break;
				case SAFTBUS_CTL_INTROSPECT:
				{
					std::string object_path, interface_name;
					saftbus::read(client_fd, object_path);
					saftbus::read(client_fd, interface_name);
					logger.add("   introspect from saftbus-ctl: object_path \'");
					logger.add(object_path);
					logger.add("\' interface_name \'");
					logger.add(interface_name);
					logger.add("\'\n");
					int saftd_object_id = _saftbus_object_ids[interface_name][object_path];
					if (_saftbus_objects.find(saftd_object_id) == _saftbus_objects.end()) {
						saftbus::write(client_fd, std::string("unknown object path"));
						break;
					}
					saftbus::write(client_fd, _saftbus_objects[saftd_object_id]->_introspection_xml);
				}
				break;
				case saftbus::SAFTBUS_CTL_GET_STATE:
				{
					// Send all mutable state variables of the Connection object.
					// This is supposed to be used by the saftbus-ctl tool.

					//std::map<std::string, std::map<std::string, int> > _saftbus_object_ids; 
					saftbus::write(client_fd, _saftbus_object_ids);

					//std::map<int, std::shared_ptr<InterfaceVTable> > _saftbus_objects;
					std::vector<int> indices;
					for (auto it: _saftbus_objects) {
						indices.push_back(it.first);
					}
					saftbus::write(client_fd, indices);

					//int _saftbus_object_id_counter; // log saftbus object creation
					saftbus::write(client_fd, _saftbus_object_id_counter);
					//int _saftbus_signal_handle_counter; // log signal subscriptions
					saftbus::write(client_fd, _saftbus_signal_handle_counter);

					//std::vector<std::shared_ptr<Socket> > _sockets; 
					std::vector<int> sockets_active;
					for (unsigned i = 0; i < _sockets.size(); ++i) {
						sockets_active.push_back(true); // sockets are always active with new socket mechanism. non-active sockets are removed immediately
					}
					saftbus::write(client_fd, sockets_active);

					saftbus::write(client_fd, _socket_owner);

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
					saftbus::write(client_fd, handle_to_signal_map);

					//std::map<std::string, std::set<unsigned> > _id_handles_map;
					saftbus::write(client_fd, _id_handles_map);

					//std::set<unsigned> erased_handles;
					std::vector<unsigned> erased_handles;
					for(auto erased_handle: _erased_handles) {
						erased_handles.push_back(erased_handle);
					}
					saftbus::write(client_fd, erased_handles);

					// store the pipes that go directly to one or many Proxy objects
							// interface_name        // object path
					//std::map<std::string, std::map < std::string , std::set< SignalFD > > > _signal_fds;
					saftbus::write(client_fd, _signal_fds);

					//static int _saftbus_id_counter;
					saftbus::write(client_fd, _saftbus_id_counter);

					// list owners
					// map object path to owner
					std::map<std::string, std::string> owners;
					auto owned_object_paths = _saftbus_object_ids["de.gsi.saftlib.Owned"];
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
					saftbus::write(client_fd, owners);

				}
				break;
				case saftbus::SENDER_ID:
				{
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SENDER_ID"]);
					logger.add("     SENDER_ID received\n");
					// std::string sender_id;
					// generate a new id value (increasing numbers)
					++_saftbus_id_counter; 
					std::ostringstream id_out;
					id_out << ":" << _saftbus_id_counter;
					_socket_owner[client_fd] = id_out.str();
					// send the id to the ProxyConnection
					saftbus::write(client_fd, _socket_owner[client_fd]);
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
					saftbus::read(client_fd, dt);
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
					saftbus::read(client_fd, object_path);
					saftbus::read(client_fd, interface_name);
					saftbus::write(client_fd, _saftbus_object_ids[interface_name][object_path]);
				}
				break;
				case saftbus::SIGNAL_FD: 
				{
					// each Proxy constructor will send the reading end of a pipe
					// to us as a fast path for signals
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_SIGNAL_FD"]);
					logger.add("     SIGNAL_FD received: ");
					int fd = saftbus::recvfd(client_fd);
					std::string name, object_path, interface_name;
					int proxy_id;
					saftbus::read(client_fd, object_path);
					saftbus::read(client_fd, interface_name);
					saftbus::read(client_fd, proxy_id);

					logger.add("for object_path=").add(object_path)
					      .add(" interface_name=").add(interface_name)
					      .add(" proxy_id=").add(proxy_id)
					      .add(" fd=").add(fd).add("\n");

					int flags = fcntl(fd, F_GETFL, 0);
					fcntl(fd, F_SETFL, flags | O_NONBLOCK);

					SignalFD pp;
					pp.id = proxy_id;
					pp.fd = fd;
					pp.socket_fd = client_fd;
					//std::cerr << "got signal fd (socketpair)" << std::endl;
					pp.con = Slib::signal_io().connect(sigc::bind(sigc::mem_fun(this, &Connection::proxy_pipe_closed), interface_name, object_path, pp), fd, Slib::IO_IN | Slib::IO_HUP, Slib::PRIORITY_LOW);
					_signal_fds[interface_name][object_path].insert(pp);

					char ch = 'x';
					//std::cerr << "writing ping: " << ch << std::endl;
					saftbus::write(fd, ch);
				}
				break;
				case saftbus::SIGNAL_REMOVE_FD: 
				{
				}
				break;
				case saftbus::METHOD_CALL: 
				{
					saftbus::Timer f_time(_function_run_times["Connection::dispatch_METHOD_CALL"]);
					logger.add("           METHOD_CALL received: ");
					
					// read the size of serialized data
					uint32_t size;
					saftbus::read(client_fd, size);
					logger.add(" message size=").add(size);

					// read the serialized data
					Serial payload;
					payload.data().resize(size);
					saftbus::read_all(client_fd, &payload.data()[0], size);

					// saftbus::Serial to get content
					int saftbus_object_id;
					std::string object_path;
					std::string sender;
					std::string interface_name;
					std::string name;
					Serial parameters;

					payload.get_init();
					payload.get(saftbus_object_id);
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
						int saftd_object_id = _saftbus_object_ids[derived_interface_name][object_path];
						if (saftd_object_id == 0 || saftd_object_id != saftbus_object_id) {// == not found or wrong check saftd_object_id
							// this causes an exception on the proxy side
							std::string what;
							if (saftd_object_id == 0) {
								what.append("unknown object path: ");
							} else {
								what.append("saftbus object under this object path has changed since Proxy creation: ");
							}
							what.append(object_path);
							saftbus::write(client_fd, saftbus::METHOD_ERROR);
							saftbus::write(client_fd, saftbus::Error::FAILED);
							saftbus::write(client_fd, what);
							logger.add(what);
							logger.log();
							break;
						}
						if (_saftbus_objects.find(saftd_object_id) == _saftbus_objects.end()) {
							std::string what;
							what.append("unknown object path: ");
							what.append(object_path);
							saftbus::write(client_fd, saftbus::METHOD_ERROR);
							saftbus::write(client_fd, saftbus::Error::FAILED);
							saftbus::write(client_fd, what);
							break;
						}
						logger.add("    found saftbus object at saftd_object_id=").add(saftd_object_id).add("\n");
						if (name == "Get") {
							logger.add("      Get the property\n");
							saftbus::Timer f_time(_function_run_times["Connection::dispatch_METHOD_CALL_GetProperty"]);

							// call auto-generated get_porperty handler using the vtable
							Serial result;
							_saftbus_objects[saftd_object_id]->get_property(result, saftbus::connection, sender, object_path, derived_interface_name, property_name);

							// prepare response data
							// serialize response data
							size = result.get_size();
							const char *data_ptr = static_cast<const char*>(result.get_data());
							// write data to socket
							logger.add("         size of response (serialized property): ").add(size).add("\n");
							saftbus::write(client_fd, saftbus::METHOD_REPLY);
							saftbus::write(client_fd, size);
							saftbus::write_all(client_fd, data_ptr, size);
							logger.add("         property was sent\n");

						} else if (name == "Set") {
							logger.add("      Set the property\n");
							std::ostringstream function_name;
							function_name << "Connection::dispatch_METHOD_CALL_SetProperty_" << derived_interface_name << "_" << property_name;
							saftbus::Timer f_time(_function_run_times[function_name.str().c_str()]);
							
							// set the value using the set_property handler from auto-generated saftlib code
							Serial par2;
							parameters.get(par2);
							try {
								bool result = _saftbus_objects[saftd_object_id]->set_property(saftbus::connection, sender, object_path, derived_interface_name, property_name, par2);

								// even the set property has a response ...
								Serial response;
								response.put(result);

								// serialize the data
								size = response.get_size();
								const char *data_ptr = static_cast<const char*>(response.get_data());

								// send the data 
								logger.add("         size of response (empty resoponse): ").add(size).add("\n");
								saftbus::write(client_fd, saftbus::METHOD_REPLY);
								saftbus::write(client_fd, size);
								saftbus::write_all(client_fd, data_ptr, size);
								logger.add("         response was sent\n");
							} catch (const saftbus::Error& error) {
								saftbus::write(client_fd, saftbus::METHOD_ERROR);
								saftbus::write(client_fd, saftbus::Error::FAILED);
								saftbus::write(client_fd, error.what());
							}
						}
					}
					else // normal method calls 
					{
						logger.add("         regular method call\n");
						std::ostringstream function_name;
						function_name << "Connection::dispatch_METHOD_CALL_" << name;
						saftbus::Timer f_time(_function_run_times[function_name.str().c_str()]);

						// saftbus object lookup
						int saftd_object_id = _saftbus_object_ids[interface_name][object_path];
						if (saftd_object_id == 0 || saftd_object_id != saftbus_object_id) {// == not found
							// this causes an exception on the proxy side
							std::string what;
							if (saftd_object_id == 0) {
								what.append("unknown object path: ");
							} else {
								what.append("saftbus object under this object path has changed since Proxy creation: ");
							}
							what.append(object_path);
							saftbus::write(client_fd, saftbus::METHOD_ERROR);
							saftbus::write(client_fd, saftbus::Error::FAILED);
							saftbus::write(client_fd, what);
							logger.add(what);
							logger.log();
							break;
						}

						std::shared_ptr<MethodInvocation> method_invocation_rptr(new MethodInvocation);

						logger.add("         calling the driver function ...\n");
						if (_saftbus_objects.find(saftd_object_id) != _saftbus_objects.end()) {
							_saftbus_objects[saftd_object_id]->method_call(saftbus::connection, sender, object_path, interface_name, name, parameters, method_invocation_rptr);
						} else {
							std::string what;
							what.append("unknown object path: ");
							what.append(object_path);
							method_invocation_rptr->return_error(saftbus::Error(saftbus::Error::ACCESS_DENIED, what));
						}
						logger.add("         ... done \n");

						if (method_invocation_rptr->has_error()) {
							logger.add("         return an error \n");
							saftbus::write(client_fd, saftbus::METHOD_ERROR);
							saftbus::write(client_fd, method_invocation_rptr->get_return_error().type());
							saftbus::write(client_fd, method_invocation_rptr->get_return_error().what());
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
							saftbus::write(client_fd, saftbus::METHOD_REPLY);
							saftbus::write(client_fd, size);
							if (size > 0) {
								saftbus::write_all(client_fd, data_ptr, size);
							}
							logger.add("         response was sent\n");
						}
					}
				}
				break;
				default:
					logger.add("      unknown message type\n");
					logger.log();
					handle_disconnect(client_fd);
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
		handle_disconnect(client_fd);
		//if (_debug_level > 5) print_all_fds();
	} catch (saftbus::Error &e) {
		std::cerr << "saftbus::Error " << e.what() << std::endl;
	}
	return false;
}

}
