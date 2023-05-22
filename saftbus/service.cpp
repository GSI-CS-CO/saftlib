/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
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

#include "service.hpp"

#include "plugins.hpp"
#include "loop.hpp"
#include "error.hpp"

#include <string>
#include <map>
#include <set>
#include <cassert>
#include <sstream>

#include <unistd.h>

namespace saftbus {

	struct Service::Impl {
		int owner;
		std::map<int, int> signal_fds_use_count;
		std::vector<std::string> interface_names;
		std::string object_path;
		uint64_t object_id;
		std::function<void()> destruction_callback; // a funtion can be attatched here that is called whenever the service is destroyed
		bool destroy_if_owner_quits; 
		void remove_signal_fd(int fd);
	};

	struct Container::Impl {
		std::vector<std::pair<std::string, std::unique_ptr<LibraryLoader> > > plugins;
		unsigned generate_saftbus_object_id();
		ServerConnection *connection;
		std::map<unsigned, std::unique_ptr<Service> > objects; // Container owns the Service objects
		std::map<std::string, unsigned> object_path_lookup_table; // maps object_path to saftbus_object_id
		std::vector<Service*> removed_services;
		std::map<std::string, std::function<std::string(void)> > additional_info_callbacks; // allow plugins to add additional info to be shown by "saftbus-ctl -s"
		void reset_children_first(const std::string &object_path) {
			if (object_path == "/saftbus") return;
			bool found_child = false;
			for (auto &obj: objects) {
				if (obj.second &&
					obj.second->d->object_path.find(object_path) == 0 && 
					obj.second->d->object_path.size() > object_path.size() && 
					obj.second->d->object_path[object_path.size()] == '/') {
					found_child = true;
					reset_children_first(obj.second->d->object_path);
				}
			}
			if (!found_child) {
				auto id = object_path_lookup_table[object_path];
				objects[id].reset();
				object_path_lookup_table.erase(object_path);
			}

		}
		void clear() {
			// Make sure that service objects are destroyed in the opposite order (youngest object first).
			// The std::map "objects" is sorted after object id, which is increasing for all created objects.
			// starting with the last object in the map, the correct destruction order is assured.
			// the lowest object_id is 1 and /saftbus has it. 
			while(objects.size()>1) {
				auto last = objects.end();
				--last;
				if (last->second->d->destruction_callback) {
					last->second->d->destruction_callback();
				}
				reset_children_first(last->second->d->object_path);
				// erase all reset-ed entries
				for (;;) {
					bool found = false;
					unsigned id;
					for (auto &obj: objects) {
						if (!obj.second) {
							id = obj.first;
							found = true;
							break;
						}
					}
					if (found) {
						objects.erase(id);
					} else {
						break;
					}
				}
			}
			object_path_lookup_table.clear();

		}
		Impl() {}
		~Impl()  {
			clear();
		}
	};



	Service::Service(const std::vector<std::string> &interface_names, std::function<void()> destruction_callback, bool destroy_if_owner_quits)
		: d(new Impl)
	{
		d->owner = -1;
		d->interface_names = interface_names;
		d->destruction_callback = destruction_callback;
		d->destroy_if_owner_quits = destroy_if_owner_quits;
	}
	Service::~Service() {
	}

	int Service::get_owner() {
		return d->owner;
	}
	bool Service::is_owned() {
		return d->owner != -1;
	}
	void Service::set_owner(int owner) {
		d->owner = owner;
	}
	void Service::release_owner() {
		d->owner = -1;
	}
	bool Service::has_destruction_callback() {
		if (d->destruction_callback) {
			return true;
		}
		return false;
	}

	// Generate the mapping from interface_name to interface_no for all interface_names,
	// return true if all interface_names are found, false otherwise
	bool Service::get_interface_name2no_map(const std::vector<std::string> &interface_names, std::map<std::string, int> &interface_name2no_map)
	{
		// Check if the requested interfaces are all implemented by this service.
		// If yes, return a map of interface_name -> interface_no for this particular service object
		bool implement_all_interfaces = true;
		for (auto &interface_name: interface_names) {
			bool interface_implemented = false;
			for (unsigned i = 0; i <  get_interface_names().size(); ++i) {
				if (interface_name == get_interface_names()[i]) {
					interface_name2no_map[interface_name] = i; // for this particular service object, i is the interface_no for interface_name
					interface_implemented = true;
				}
			}
			if (!interface_implemented) {
				implement_all_interfaces = false;
			}
		}
		return implement_all_interfaces;
	}

	void Service::call(int client_fd, Deserializer &received, Serializer &send) {
		int interface_no, function_no;
		received.get(interface_no);
		received.get(function_no);
		call(interface_no, function_no, client_fd, received, send);
	}

	void Service::Impl::remove_signal_fd(int fd)
	{
		auto found_use_count = signal_fds_use_count.find(fd);
		if (found_use_count != signal_fds_use_count.end()) {
			signal_fds_use_count.erase(fd);
		}
	}

	void Service::emit(Serializer &send)
	{
		for (auto &fd_use_count: d->signal_fds_use_count) {
			if (fd_use_count.second > 0) { // only send data if use count is > 0
				int fd = fd_use_count.first;
				struct pollfd pfd;
				pfd.fd = fd;
				pfd.events = POLLOUT;
				if (poll(&pfd, 1, 10) <= 0) { // fd is not ready after 10 ms drop the signal (write to stderr?)
					// std::cerr << "Service " << get_object_path() << "Service::emit() drop signal for fd " << fd << std::endl;
				} else {
					send.write_to_no_init(fd); // The same data is written multiple times. Therefore the
				}                             // put_init function must not be called automatically after write
			}                                //
		}                                   // but manually after the for loop 
		send.put_init();                   // <- here
	}

	int Service::get_object_id() 
	{
		return d->object_id;
	}




	std::string &Service::get_object_path()
	{
		return d->object_path;
	}
	std::vector<std::string> &Service::get_interface_names() {
		return d->interface_names;
	}

	// created by saftbus-gen from class Container and copied here
	std::vector<std::string> Container_Service::gen_interface_names() {
		std::vector<std::string> result; 
		result.push_back("Container");
		return result;
	}
	Container_Service::Container_Service(Container* instance, std::function<void()> destruction_callback) 
	: saftbus::Service(gen_interface_names(), destruction_callback), d(instance)
	{
	}
	Container_Service::~Container_Service() 
	{
	}
	void Container_Service::call(unsigned interface_no, unsigned function_no, int client_fd, saftbus::Deserializer &received, saftbus::Serializer &send) {
		try {
		switch(interface_no) {
			case 0: // Container
			switch(function_no) {
				case 0: { // Container::register_proxy (Hand-written. It will be called by Poxy base class constructor)
					std::string object_path;
					received.get(object_path);
					std::vector<std::string> interface_names;
					received.get(interface_names);
					int signal_fd;
					received.get(signal_fd);


					if (signal_fd == -1) {
						signal_fd = recvfd(client_fd);
						// std::cerr << "got (open) " << signal_fd << std::endl;
					} else {
						// std::cerr << "reuse " << signal_fd << std::endl;
					}
					std::map<std::string, int> interface_name2no_map;
					unsigned saftbus_object_id = d->register_proxy(object_path, interface_names, interface_name2no_map, client_fd, signal_fd);
					send.put(saftbus_object_id);
					send.put(client_fd); // fd and signal_fd are used in the proxy de-registration process
					send.put(signal_fd); // send the integer value of the signal_fd back to the proxy. This nuber can be used by other Proxies to reuse the signal pipe.
					send.put(interface_name2no_map);
				} return;
				case 1: { // Container::unregister_proxy (Hand-written. It will be called by Proxy base class destructor)
					unsigned saftbus_object_id;
					int received_client_fd, received_signal_group_fd;
					received.get(saftbus_object_id);
					received.get(received_client_fd);
					received.get(received_signal_group_fd);
					d->unregister_proxy(saftbus_object_id, received_client_fd, received_signal_group_fd);
				} return;
				case 2: { // Container::load_plugin
					std::string  so_filename;
					received.get(so_filename);
					std::vector<std::string> plugin_args;
					received.get(plugin_args);
					bool function_call_result = d->load_plugin(so_filename, plugin_args);
					send.put(saftbus::FunctionResult::RETURN);
					send.put(function_call_result);
				} return;
				case 3: { // Container::unload_plugin
					std::string  so_filename;
					received.get(so_filename);
					std::vector<std::string> plugin_args;
					received.get(plugin_args);
					bool function_call_result = d->unload_plugin(so_filename, plugin_args);
					send.put(saftbus::FunctionResult::RETURN);
					send.put(function_call_result);
				} return;
				case 4: { // Container::remove_object  // this is largely hand written, because we have to make sure that the destruction callback is correctly executed
					std::string  object_path;
					received.get(object_path);
					bool function_call_result = true;
					Service* service = d->removal_helper(object_path);
					if (service->d->destruction_callback) {
						service->d->destruction_callback();
					}
					if (d->d->object_path_lookup_table.find(object_path) != d->d->object_path_lookup_table.end()) { // it may be that destruction callback already removed the object
						d->remove_object(object_path);
					}
					send.put(saftbus::FunctionResult::RETURN);
					send.put(function_call_result);
				} return;
				case 5: { // Container::quit
					d->quit();
					send.put(saftbus::FunctionResult::RETURN);
				} return;
				case 6: { // Container::get_status
					SaftbusInfo function_call_result = d->get_status();
					send.put(saftbus::FunctionResult::RETURN);
					send.put(function_call_result);
				} return;
			};

		};
		} catch (std::runtime_error &e) {
			send.put(saftbus::FunctionResult::EXCEPTION);
			std::string what(e.what());
			send.put(what);
		} catch (...) {
			send.put(saftbus::FunctionResult::EXCEPTION);
			std::string what("unknown exception");
			send.put(what);
		} 
	}


	// generate a unique object_id != 0
	unsigned Container::Impl::generate_saftbus_object_id() {
		static unsigned saftbus_object_id_generator = 1;
		while ((objects.find(saftbus_object_id_generator) != objects.end()) ||
		        saftbus_object_id_generator == 0) {
			++saftbus_object_id_generator;
		}
		return saftbus_object_id_generator++;
	}

	Container::Container(ServerConnection *connection) 
		: d(new Impl)
	{
		unsigned object_id = create_object("/saftbus", std::move(std::unique_ptr<Container_Service>(new Container_Service(this))));
		assert(object_id == 1); // the entier system relies on having Container_Service at object_id 1	
		d->connection = connection;
		// d->active_service = nullptr;
	}

	Container::~Container() 
	{
	}

	unsigned Container::create_object(const std::string &object_path, std::unique_ptr<Service> service)
	{
		if (d->object_path_lookup_table.find(object_path) != d->object_path_lookup_table.end()) {
			// we have already registered an object under this object path
			return 0;
		}
		unsigned saftbus_object_id = d->generate_saftbus_object_id();
		service->d->object_id = saftbus_object_id;
		auto insertion_result = d->objects.insert(std::make_pair(saftbus_object_id, std::move(service)));
		auto  insertion_took_place  = insertion_result.second;
		auto &inserted_object       = insertion_result.first->second; 
		if (insertion_took_place) {
			inserted_object->d->object_path = object_path; // set the object_path of the Service object
			d->object_path_lookup_table[object_path] = saftbus_object_id;
			return saftbus_object_id;
		}
		return 0;
	}

	Service* Container::get_object(const std::string &object_path)
	{
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result == d->object_path_lookup_table.end()) {
			std::string msg = "cannot get object because its object_path \"";
			msg.append(object_path);
			msg.append("\" was not found");
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg);
		}
		auto object_id = find_result->second;
		auto &service = d->objects[object_id];
		return service.get();
	}

	void Container::destroy_service(Service *service) {
		if (get_calling_client_id() != -1) {
			d->removed_services.push_back(service);
		} else {
			try {
				remove_object(service->d->object_path);
			} catch(std::runtime_error &e) {
				std::cerr << "Exception in " << __FUNCTION__ << " : " << e.what() << std::endl;
			}
		}
	}

	Service* Container::removal_helper(const std::string &object_path)
	{
		if (object_path == "/saftbus") {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "cannot remove /saftbus");
		}
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result == d->object_path_lookup_table.end()) {
			std::string msg = "cannot remove object \"";
			msg.append(object_path);
			msg.append("\" because its object_path was not found");
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg);
		}
		auto object_id = find_result->second;
		auto &service = d->objects[object_id];
		if (service->d->owner != -1) { // the service is owned
			if (service->d->owner != d->connection->get_calling_client_id()) {
				std::ostringstream msg;
				msg << "cannot remove object \"" << object_path << "\" because it owned by other client " << service->d->owner;
				throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg.str());
			}
		}

		// make sure that non of the children of the object to be removed has a foreign owner
		// the child relationship is determined only based on the object path
		// it is in the responability of the Service developer that all child 
		// relationships are properly reflected by the object paths.
		for (auto &object: d->object_path_lookup_table) {
			auto &other_path = object.first;
			if (other_path == "/saftbus") {
				continue;
			}
			if (other_path != object_path &&              // check if other_path differs
			    other_path.find(object_path)==0 &&        // check if other_path starts with object_path
			    other_path.size() > object_path.size() && // check if other_path is longer then object_path (child object paths are always longer)
			    other_path[object_path.size()] == '/' ) { // and the first non-commom character in a child path is '/', as in "/parent/child".
				auto object_id = object.second;
				auto &service = d->objects[object_id];
				if (service->d->owner != -1) { // the service is owned
					if (service->d->owner != d->connection->get_calling_client_id()) {
						std::ostringstream msg;
						msg << "cannot remove object \"" << object_path << "\" because child object \"" << other_path << "\" is owned by other client " << service->d->owner;
						throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg.str());
					}
				}
			}
		}
		return d->objects[d->object_path_lookup_table[object_path]].get();
	}

	bool Container::remove_object(const std::string &object_path)
	{
		removal_helper(object_path);
		auto object_id = d->object_path_lookup_table[object_path];
		d->object_path_lookup_table.erase(object_path);
		d->objects.erase(object_id);
		return false;
	}


	int Container::register_proxy(const std::string &object_path, const std::vector<std::string> interface_names, std::map<std::string, int> &interface_name2no_map, int client_fd, int signal_group_fd)
	{
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result != d->object_path_lookup_table.end()) {
			unsigned saftbus_object_id = find_result->second;
			auto find_result = d->objects.find(saftbus_object_id);
			assert(find_result != d->objects.end()); // if this cannot be found, the lookup table is not correct
			auto &service    = find_result->second;
			if (service->get_interface_name2no_map(interface_names, interface_name2no_map)) { //returns false if not all requested interfaces are implemented
				service->d->signal_fds_use_count[signal_group_fd]++;
				d->connection->register_signal_id_for_client(client_fd, signal_group_fd);
				return saftbus_object_id;
			}
			// not all requested interfaces are implemented => return -1
			return -1;
		}
		return 0;
	}
	void Container::unregister_proxy(unsigned saftbus_object_id, int client_fd, int signal_group_fd)
	{
		auto find_result = d->objects.find(saftbus_object_id);
		if (find_result == d->objects.end()) {
			// std::cerr << "object id " << saftbus_object_id << " already gone" << std::endl;
			return;
		}
		auto    &service    = find_result->second;
		service->d->signal_fds_use_count[signal_group_fd]--;
		d->connection->unregister_signal_id_for_client(client_fd, signal_group_fd);
		if (service->d->signal_fds_use_count[signal_group_fd] == 0) {
			service->d->signal_fds_use_count.erase(signal_group_fd);
		}
	}


	bool Container::call_service(unsigned saftbus_object_id, int client_fd, Deserializer &received, Serializer &send) {
		auto find_result = d->objects.find(saftbus_object_id);
		if (find_result == d->objects.end()) {
			return false;
		}
		auto &service = find_result->second;
		service->call(client_fd, received, send);

		for (auto &s: d->removed_services) {
			if (s->d->destruction_callback) {
				s->d->destruction_callback();
			}
			try {
				remove_object(s->d->object_path);
			} catch(std::runtime_error &e) {
				std::cerr << "Exception in " << __FUNCTION__ << " : " << e.what() << std::endl;
			}
		}
		d->removed_services.clear();

		return true;
	}

	void Container::remove_signal_fd(int fd)
	{
		for(auto &service: d->objects) {
			service.second->d->remove_signal_fd(fd);
		}
	}


	// operator is used to std::find a Service based on owner, where the owner is identified by file descriptor (fd) of the client socket
	bool operator==(std::pair<const unsigned int, std::unique_ptr<saftbus::Service> > &p, const int fd) {
		return p.second->d->owner == fd;
	}
	void Container::client_hung_up(int fd) {
		// There may be parent-child relations between service objects it must be ensured 
		// that children are always destroyed before their parents.
		// All service objects get an object id which is always increasing.
		// Thus, children always have larger numbers then their parents.
		// Thus a viable strategy to remove children before parents is
		// the use of reverse iterators when selecting the owned service objects.
		// This works because iterating a map with object ids as key value is sorted.
		for(;;) {
			auto iter = std::find(d->objects.rbegin(), d->objects.rend(), fd);
			if (iter == d->objects.rend()) {
				break;
			}
			if (iter->second->d->destruction_callback) {
				iter->second->d->destruction_callback();
			}
			if (iter->second->d->destruction_callback && iter->second->d->destroy_if_owner_quits) {
				try {
					remove_object(iter->second->get_object_path());
				} catch(std::runtime_error &e) {
					std::cerr << "Exception in " << __FUNCTION__ << " : " << e.what() << std::endl;
				}
			} else {
				// if there is no destruction_callback, release object from clients ownership (because the client hung up)
				iter->second->d->owner = -1;
			}
		} 
	}


	bool Container::load_plugin(const std::string &so_filename, const std::vector<std::string> &args) {
		bool plugin_available = false;
		LibraryLoader *plugin = nullptr;
		for (auto &name_plugin: d->plugins) {
			if (name_plugin.first == so_filename) {
				plugin_available = true;
				plugin = name_plugin.second.get();
				break;
			}
		}
		if (!plugin_available) {			
			d->plugins.push_back(std::make_pair(so_filename, std::move(std::unique_ptr<LibraryLoader>(new LibraryLoader(so_filename)))));
			plugin = d->plugins.back().second.get();
			plugin_available = true;
		}
		if (plugin_available) {
			plugin->create_services(this, args);
		}
		return plugin_available;
	}

	bool Container::unload_plugin(const std::string &so_filename, const std::vector<std::string> &args) {
		if (d->plugins.size() > 0 && d->plugins.back().first == so_filename) {
			d->plugins.pop_back();
			return true;
		}
		return false;
	}

	void Container::quit() {
		d->clear();
		saftbus::Loop::get_default().quit();
	}

	int Container::get_calling_client_id() const {
		return d->connection->get_calling_client_id();
	}

	void Container::clear() {
		d->clear();
	}


	void Container::add_additional_info_callback(const std::string &name, std::function<std::string(void)> callback) {
		d->additional_info_callbacks[name] = callback;
	}
	void Container::remove_additional_info_callback(const std::string &name) {
		d->additional_info_callbacks.erase(name);
	}


	SaftbusInfo Container::get_status() {
		SaftbusInfo result;
		for (auto &obj: d->objects) {
			SaftbusInfo::ObjectInfo object_info;
			object_info.object_id = obj.first;
			object_info.object_path = obj.second->d->object_path;
			object_info.interface_names = obj.second->d->interface_names;
			object_info.signal_fds_use_count = obj.second->d->signal_fds_use_count;
			object_info.owner = obj.second->d->owner;
			object_info.has_destruction_callback = obj.second->d->destruction_callback?true:false;
			object_info.destroy_if_owner_quits = obj.second->d->destroy_if_owner_quits;
			result.object_infos.push_back(object_info);
		}
		for (auto &client: d->connection->get_client_info()) {
			SaftbusInfo::ClientInfo client_info;
			client_info.process_id = client.process_id;
			client_info.client_fd  = client.client_fd;
			client_info.signal_fds = client.signal_fds;
			result.client_infos.push_back(client_info);
		}
		for (auto &name_loader: d->plugins) {
			result.active_plugins.push_back(name_loader.first);
		}
		for (auto &additional: d->additional_info_callbacks) {
			result.additional_info[additional.first] = additional.second();
		}

		return result;
	}


}
