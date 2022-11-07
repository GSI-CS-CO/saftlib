#include "service.hpp"

#include "make_unique.hpp"
#include "plugins.hpp"
#include "loop.hpp"
#include "error.hpp"

#include <string>
#include <map>
#include <set>
#include <cassert>

#include <unistd.h>

namespace saftbus {

	struct Service::Impl {
		int owner;
		std::map<int, int> signal_fds_use_count;
		std::vector<std::string> interface_names;
		std::string object_path;
		uint64_t object_id;
		std::function<void()> destruction_callback; // a funtion can be attatched here that is called whenever the service is destroyed
		bool active_service_remove; // initialized to false, only set to true by Container::active_service_remove() to allow services to remove themselves safely
		void remove_signal_fd(int fd);
		~Impl()  {
			std::cerr << "Service::~Impl(" << object_path << ")" << std::endl;
			if (destruction_callback) {
				std::cerr << "Service::~Impl() -> destruction_callback" << std::endl;
				destruction_callback();
			}
		}
	};

	struct Container::Impl {
		std::map<std::string, std::unique_ptr<LibraryLoader> > plugins;
		unsigned generate_saftlib_object_id();
		ServerConnection *connection;
		std::map<unsigned, std::unique_ptr<Service> > objects; // Container owns the Service objects
		std::map<std::string, unsigned> object_path_lookup_table; // maps object_path to saftlib_object_id
		Service *active_service; // this is set only during the call_service function
		void erase_children_first(const std::string &object_path) {
			bool found_child = false;
			for (auto &obj: objects) {
				if (obj.second->d->object_path.find(object_path) == 0 && 
					obj.second->d->object_path.size() > object_path.size() && 
					obj.second->d->object_path[object_path.size()] == '/') {
					found_child = true;
					erase_children_first(obj.second->d->object_path);
				}
			}
			if (!found_child) {
				auto id = object_path_lookup_table[object_path];
				object_path_lookup_table.erase(object_path);
				objects.erase(id);
			}
		}
		Impl() {}
		~Impl()  {
			std::cerr << "Container::~Impl()" << std::endl;
			// Make sure that service objects are destroyed in the opposite order (youngest object first).
			// The std::map "objects" is sorted after object id, which is increasing for all created objects.
			// starting with the last object in the map, the correct destruction order is assured.
			while(objects.size()) {
				auto last = objects.end();
				--last;
				erase_children_first(last->second->d->object_path);
			}
		}
	};

	// struct Container_Service::Impl {
	// 	Container *container;
	// 	Serializer serialized_signal;
	// 	static std::vector<std::string> gen_interface_names();
	// 	~Impl()  {
	// 		std::cerr << "Container_Service::~Impl()" << std::endl;
	// 	}
	// };


	Service::Service(const std::vector<std::string> &interface_names, std::function<void()> destruction_callback)
		: d(std2::make_unique<Impl>())
	{
		d->owner = -1;
		d->interface_names = interface_names;
		d->destruction_callback = destruction_callback;
		d->active_service_remove = false;  
	}
	Service::~Service() {
		std::cerr << "~Service " << d->object_path << std::endl;
		// std::cerr << "~Service done" << std::endl;
	}

	// Generate the mapping from interface_name to interface_no for all interface_names,
	// return true if all interface_names are found, false otherwise
	bool Service::get_interface_name2no_map(const std::vector<std::string> &interface_names, std::map<std::string, int> &interface_name2no_map)
	{
		// Check if the requested interfaces are all implemented by this service.
		// If yes, return a map of interface_name -> interface_no for this particular service object
		bool implement_all_interfaces = true;
		for (auto &interface_name: interface_names) {
			// std::cerr << "  check for interface " << interface_name << std::endl;
			bool interface_implemented = false;
			for (unsigned i = 0; i <  get_interface_names().size(); ++i) {
				// std::cerr << "    check against " << get_interface_names()[i] << std::endl;
				if (interface_name == get_interface_names()[i]) {
					interface_name2no_map[interface_name] = i; // for this particular service object, i is the interface_no for interface_name
					interface_implemented = true;
				}
			}
			// std::cerr << "  => " << interface_implemented << std::endl;
			if (!interface_implemented) {
				implement_all_interfaces = false;
				// std::cerr << "requested interface " << interface_name << "is not implemented by " << get_object_path() << std::endl;
			}
		}
		return implement_all_interfaces;
	}

	void Service::call(int client_fd, Deserializer &received, Serializer &send) {
		int interface_no, function_no;
		received.get(interface_no);
		received.get(function_no);
		// std::cerr << "Service::call got " << interface_no << " " << function_no << std::endl;
		call(interface_no, function_no, client_fd, received, send);
	}

	void Service::Impl::remove_signal_fd(int fd)
	{
		std::cerr << "Service::Impl::remove_signal_fd " << fd << std::endl;
		auto found_use_count = signal_fds_use_count.find(fd);
		if (found_use_count != signal_fds_use_count.end()) {
			signal_fds_use_count.erase(fd);
		}
		std::cerr << " number of signal fds: " << signal_fds_use_count.size() << std::endl;
	}

	void Service::emit(Serializer &send)
	{
		// std::cerr << "emitting signal. number of signal fds: " << d->signal_fds_use_count.size() << std::endl;
		for (auto &fd_use_count: d->signal_fds_use_count) {
			if (fd_use_count.second > 0) { // only send data if use count is > 0
				int fd = fd_use_count.first;
				std::cerr << "   to " << fd << std::endl;
				send.write_to_no_init(fd); // The same data is written multiple times. Therefore the
				                          // put_init function must not be called automatically after write
			}                            //
		}                               // but manually after the for loop 
		send.put_init();               // <- here
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
					std::cerr << "register_proxy called" << std::endl;
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
					unsigned saftlib_object_id = d->register_proxy(object_path, interface_names, interface_name2no_map, client_fd, signal_fd);
					std::cerr << "registered proxy for saftlib_object_id " << saftlib_object_id << std::endl;
					send.put(saftlib_object_id);
					send.put(client_fd); // fd and signal_fd are used in the proxy de-registration process
					send.put(signal_fd); // send the integer value of the signal_fd back to the proxy. This nuber can be used by other Proxies to reuse the signal pipe.
					send.put(interface_name2no_map);
				} return;
				case 1: { // Container::unregister_proxy (Hand-written. It will be called by Proxy base class destructor)
					std::cerr << "unregister_proxy called" << std::endl;
					unsigned saftlib_object_id;
					int received_client_fd, received_signal_group_fd;
					received.get(saftlib_object_id);
					received.get(received_client_fd);
					received.get(received_signal_group_fd);
					d->unregister_proxy(saftlib_object_id, received_client_fd, received_signal_group_fd);
				} return;
				case 2: { // Container::load_plugin
					std::string  so_filename;
					received.get(so_filename);
					bool function_call_result = d->load_plugin(so_filename);
					send.put(saftbus::FunctionResult::RETURN);
					send.put(function_call_result);
				} return;
				case 3: { // Container::remove_object
					std::string  object_path;
					received.get(object_path);
					bool function_call_result = d->remove_object(object_path);
					send.put(saftbus::FunctionResult::RETURN);
					send.put(function_call_result);
				} return;
				case 4: { // Container::quit
					d->quit();
					send.put(saftbus::FunctionResult::RETURN);
				} return;
				case 5: { // Container::get_status
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
	unsigned Container::Impl::generate_saftlib_object_id() {
		static unsigned saftlib_object_id_generator = 1;
		while ((objects.find(saftlib_object_id_generator) != objects.end()) ||
		        saftlib_object_id_generator == 0) {
			++saftlib_object_id_generator;
		}
		return saftlib_object_id_generator++;
	}

	Container::Container(ServerConnection *connection) 
		: d(std2::make_unique<Impl>())
	{
		unsigned object_id = create_object("/saftbus", std::move(std::unique_ptr<Container_Service>(new Container_Service(this))));
		assert(object_id == 1); // the entier system relies on having CoreService at object_id 1	
		d->connection = connection;
		d->active_service = nullptr;
	}

	Container::~Container() 
	{
		std::cerr << "~Container" << std::endl;
	}

	unsigned Container::create_object(const std::string &object_path, std::unique_ptr<Service> service)
	{
		if (d->object_path_lookup_table.find(object_path) != d->object_path_lookup_table.end()) {
			// we have already registered an object under this object path
			std::cerr << "object path " << object_path << " already in use by object_id " << d->object_path_lookup_table[object_path] << ". cannot register another object under the same path" << std::endl;
			return 0;
		}
		unsigned saftlib_object_id = d->generate_saftlib_object_id();
		service->d->object_id = saftlib_object_id;
		auto insertion_result = d->objects.insert(std::make_pair(saftlib_object_id, std::move(service)));
		auto  insertion_took_place  = insertion_result.second;
		auto &inserted_object       = insertion_result.first->second; 
		if (insertion_took_place) {
			inserted_object->d->object_path = object_path; // set the object_path of the Service object
			d->object_path_lookup_table[object_path] = saftlib_object_id;
			std::cerr << "inserted object under object_path " << object_path << " with object_id " << saftlib_object_id << std::endl;
			return saftlib_object_id;
		}
		return 0;
	}

	bool Container::remove_object(const std::string &object_path)
	{
		if (object_path == "/saftbus") {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "cannot remove /saftbus");
		}
		// std::cerr << "remove_object " << object_path << " now" << std::endl;
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result == d->object_path_lookup_table.end()) {
			std::string msg = "cannot remove object \"";
			msg.append(object_path);
			msg.append("\" because its object_path was not found");
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg);
		}
		auto object_id = find_result->second;
		std::cerr << "Container::remove_object remove object id " << object_id << std::endl;
		std::cerr << "all objects " << std::endl;
		for (auto &obj: d->objects) {
			std::cerr << "    " << obj.first << " owner " << obj.second->d->owner << std::endl;
		}
		auto &service = d->objects[object_id];
		if (service->d->owner != -1) { // the service is owned
			if (service->d->owner != d->connection->get_calling_client_id()) {
				std::ostringstream msg;
				msg << "cannot remove object \"" << object_path << "\" because it owned by other client" << service->d->owner;
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

		d->object_path_lookup_table.erase(object_path);
		d->objects.erase(object_id);
		return false;
	}

	int Container::register_proxy(const std::string &object_path, const std::vector<std::string> interface_names, std::map<std::string, int> &interface_name2no_map, int client_fd, int signal_group_fd)
	{
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result != d->object_path_lookup_table.end()) {
			unsigned saftlib_object_id = find_result->second;
			auto find_result = d->objects.find(saftlib_object_id);
			assert(find_result != d->objects.end()); // if this cannot be found, the lookup table is not correct
			auto &service    = find_result->second;
			if (service->get_interface_name2no_map(interface_names, interface_name2no_map)) { //returns false if not all requested interfaces are implemented
				service->d->signal_fds_use_count[signal_group_fd]++;
				std::cerr << "register_proxy for object path " << object_path << " . object use count = " << service->d->signal_fds_use_count[signal_group_fd] << std::endl;
				d->connection->register_signal_id_for_client(client_fd, signal_group_fd);
				return saftlib_object_id;
			}
			// not all requested interfaces are implmented => return -1
			return -1;
		}
		return 0;
	}
	void Container::unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd)
	{
		std::cerr << "Container::unregister_proxy(" << saftlib_object_id << ")" << std::endl;
		auto find_result = d->objects.find(saftlib_object_id);
		if (find_result == d->objects.end()) {
			std::cerr << "object " << saftlib_object_id << " already gone" << std::endl;
			return;
		}
		auto    &service    = find_result->second;
		service->d->signal_fds_use_count[signal_group_fd]--;
		d->connection->unregister_signal_id_for_client(client_fd, signal_group_fd);
		std::cerr << "unregister_proxy: signal fd " << signal_group_fd << " use count = " << service->d->signal_fds_use_count[signal_group_fd] << std::endl;
		if (service->d->signal_fds_use_count[signal_group_fd] == 0) {
			service->d->signal_fds_use_count.erase(signal_group_fd);
		}
	}


	bool Container::call_service(unsigned saftlib_object_id, int client_fd, Deserializer &received, Serializer &send) {
		auto find_result = d->objects.find(saftlib_object_id);
		if (find_result == d->objects.end()) {
			return false;
		}
		auto &service = find_result->second;
		d->active_service = service.get();
		service->call(client_fd, received, send);

		if (d->active_service->d->active_service_remove) { // if the service marked itself as removed by calling Container::active_service_remove()
			std::cerr << "active_service_remove true for " << d->active_service->d->object_path << std::endl;
			remove_object(d->active_service->d->object_path);
		}

		d->active_service = nullptr;
		return true;
	}

	void Container::remove_signal_fd(int fd)
	{
		for(auto &service: d->objects) {
			// std::cerr << "remove_signal_fd(" << fd << ") for object " << service.second->d->object_path << std::endl;
			service.second->d->remove_signal_fd(fd);
		}
	}


	// define this operator to find a Service based on owner 
	bool operator==(std::pair<const unsigned int, std::unique_ptr<saftbus::Service> > &p, const int fd) {
		return p.second->d->owner == fd;
	}
	void Container::client_hung_up(int fd) {
		std::cerr << "Container::client_hung_up(" << fd << ")" << std::endl;
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
			std::cerr << "remove object " << iter->second->get_object_path() << std::endl;
			if (iter->second->d->destruction_callback) {
				// only remove those objects with a destruction_callback
				remove_object(iter->second->get_object_path());
			} else {
				// if there is no destruction_callback, release object from clients ownership (because the client hung up)
				iter->second->d->owner = -1;
			}
		} 
	}


	bool Container::load_plugin(const std::string &so_filename, const std::vector<std::string> &args) {
		std::cerr << "loading " << so_filename << std::endl;
		bool plugin_available = false;
		auto plugin = d->plugins.find(so_filename);
		if (plugin != d->plugins.end()) {
			std::cerr << "plugin found" << std::endl;
			plugin_available = true;
		} else {
			auto insertion_result = d->plugins.insert(std::make_pair(so_filename, std::move(std2::make_unique<LibraryLoader>(so_filename))));
			if (insertion_result.second) {
				std::cerr << "plugin inserted" << std::endl;
				plugin = insertion_result.first;
				plugin_available = true;
			} else {
				std::cerr << "plugin insertion failed" << std::endl;
			}
		}
		if (plugin_available) {
			plugin->second->create_services(this, args);
		}
		return plugin_available;
	}

	void Container::quit() {
		saftbus::Loop::get_default().quit();
	}

	int Container::get_calling_client_id() const {
		return d->connection->get_calling_client_id();
	}
	void Container::set_owner(Service *s) {
		if (s->d->owner != -1 && s->d->owner != get_calling_client_id()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Already have an Owner");
		}
		s->d->owner = get_calling_client_id();
	}
	void Container::active_service_set_owner() {
		if (d->active_service) {
			if (d->active_service->d->owner != -1 && d->active_service->d->owner != get_calling_client_id()) {
				throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Already have an Owner");
			}
			d->active_service->d->owner = get_calling_client_id();
		}
	}
	int Container::active_service_get_owner() const {
		if (d->active_service) {
			return d->active_service->d->owner;
		}
		return -1;
	}
	void Container::active_service_release_owner() {
		if (d->active_service) {
			if (d->active_service->d->owner == -1) {
				throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Do not have an Owner");
			}
			active_service_owner_only();
			d->active_service->d->owner = -1;
		}
	}
	void Container::active_service_owner_only() const {
		if (d->active_service) {
			// std::cout << "owner: " << d->active_service->d->owner << "            caller: " << get_calling_client_id() << std::endl;
			if (d->active_service->d->owner != -1 && d->active_service->d->owner != get_calling_client_id()) { 
				throw saftbus::Error(saftbus::Error::INVALID_ARGS, "You are not my Owner");
			}
		}
	}
	bool Container::active_service_has_destruction_callback() const {
		if (d->active_service) {
			if (d->active_service->d->destruction_callback) {
				return true;
			} 
		}
		return false;
	}
	void Container::active_service_remove() {
		if (d->active_service) {
			d->active_service->d->active_service_remove = true;
		}
	}

	void Container::clear() {
		d->objects.clear();
		d->object_path_lookup_table.clear();
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
			object_info.has_desctruction_callback = obj.second->d->destruction_callback?true:false;
			result.object_infos.push_back(object_info);
		}
		for (auto &client: d->connection->get_client_info()) {
			SaftbusInfo::ClientInfo client_info;
			client_info.process_id = client.process_id;
			client_info.client_fd  = client.client_fd;
			client_info.signal_fds = client.signal_fds;
			result.client_infos.push_back(client_info);
		}

		return result;
	}


}
