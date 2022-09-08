#include "service.hpp"

#include "make_unique.hpp"
#include "plugins.hpp"
#include "loop.hpp"

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
		unsigned object_id;
		void remove_signal_fd(int fd);
	};

	struct Container::Impl {
		unsigned generate_saftlib_object_id();
		ServerConnection *connection;
		std::map<unsigned, std::unique_ptr<Service> > objects;
		std::map<std::string, unsigned> object_path_lookup_table; // maps object_path to saftlib_object_id
		std::string delayed_removal_object_path;
	};

	struct Container_Service::Impl {
		Container *container;
		Serializer serialized_signal;
		static std::vector<std::string> gen_interface_names();
		std::map<std::string, std::unique_ptr<LibraryLoader> > plugins;
	};


	Service::Service(const std::vector<std::string> &interface_names) 
		: d(std2::make_unique<Impl>())
	{
		d->owner = -1;
		d->interface_names = interface_names;
	}
	Service::~Service() = default;

	// Generate the mapping from interface_name to interface_no for all interface_names,
	// return true if all interface_names are found, false otherwise
	bool Service::get_interface_name2no_map(const std::vector<std::string> &interface_names, std::map<std::string, int> &interface_name2no_map)
	{
		// Check if the requested interfaces are all implemented by this service.
		// If yes, return a map of interface_name -> interface_no for this particular service object
		bool implement_all_interfaces = true;
		for (auto &interface_name: interface_names) {
			std::cerr << "  check for interface " << interface_name << std::endl;
			bool interface_implemented = false;
			for (unsigned i = 0; i <  get_interface_names().size(); ++i) {
				std::cerr << "    check against " << get_interface_names()[i] << std::endl;
				if (interface_name == get_interface_names()[i]) {
					interface_name2no_map[interface_name] = i; // for this particular service object, i is the interface_no for interface_name
					interface_implemented = true;
				}
			}
			std::cerr << "  => " << interface_implemented << std::endl;
			if (!interface_implemented) {
				implement_all_interfaces = false;
				std::cerr << "requested interface " << interface_name << "is not implemented by " << get_object_path() << std::endl;
			}
		}
		return implement_all_interfaces;
	}

	void Service::call(int client_fd, Deserializer &received, Serializer &send) {
		int interface_no, function_no;
		received.get(interface_no);
		received.get(function_no);
		std::cerr << "Service::call got " << interface_no << " " << function_no << std::endl;
		call(interface_no, function_no, client_fd, received, send);
	}

	void Service::Impl::remove_signal_fd(int fd)
	{
		// std::cerr << "Service::Impl::remove_signal_fd " << fd << std::endl;
		auto found_use_count = signal_fds_use_count.find(fd);
		if (found_use_count != signal_fds_use_count.end()) {
			signal_fds_use_count.erase(fd);
		}
		// std::cerr << " number of signal fds: " << signal_fds_use_count.size() << std::endl;
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



	std::vector<std::string> Container_Service::Impl::gen_interface_names() {
		std::vector<std::string> interface_names;
		interface_names.push_back("Container");
		return interface_names;
	}

	Container_Service::Container_Service(Container *container)
		: Service(Impl::gen_interface_names())
		, d(std2::make_unique<Impl>())
	{
		d->container = container;
		// Loop::get_default().connect<saftbus::TimeoutSource>(
		// 	std::bind(&Container_Service::emit_periodical_signal, this), std::chrono::milliseconds(1000)
		// );

	}
	Container_Service::~Container_Service() = default;

	bool Container_Service::emit_periodical_signal() {
		static int count = 0;
		d->serialized_signal.put(get_object_id());
		int interface_no = 0;
		d->serialized_signal.put(interface_no);
		int signal_no = 0;
		d->serialized_signal.put(signal_no);
		d->serialized_signal.put(++count);
		// std::cerr << "emit signal with counter value " << count << std::endl;
		emit(d->serialized_signal);
		return true;
	}

	void Container_Service::call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) {
		std::cerr << "Container_Service called with " << interface_no << " " << function_no << std::endl;
		if (interface_no == 0) {
			switch(function_no) {
				case 0: {// register proxy
					std::cerr << "register_proxy called" << std::endl;
					std::string object_path;
					received.get(object_path);
					std::vector<std::string> interface_names;
					received.get(interface_names);
					int signal_fd;
					received.get(signal_fd);


					if (signal_fd == -1) {
						signal_fd = recvfd(client_fd);
						std::cerr << "got (open) " << signal_fd << std::endl;
					} else {
						std::cerr << "reuse " << signal_fd << std::endl;
					}
					std::map<std::string, int> interface_name2no_map;
					unsigned saftlib_object_id = d->container->register_proxy(object_path, interface_names, interface_name2no_map, client_fd, signal_fd);
					std::cerr << "registered proxy for saftlib_object_id " << saftlib_object_id << std::endl;
					send.put(saftlib_object_id);
					send.put(client_fd); // fd and signal_fd are used in the proxy de-registration process
					send.put(signal_fd); // send the integer value of the signal_fd back to the proxy. This nuber can be used by other Proxies to reuse the signal pipe.
					send.put(interface_name2no_map);
				}
				break;
				case 1: {// unregister proxy;
					std::cerr << "unregister_proxy called" << std::endl;
					unsigned saftlib_object_id;
					int received_client_fd, received_signal_group_fd;
					received.get(saftlib_object_id);
					received.get(received_client_fd);
					received.get(received_signal_group_fd);
					d->container->unregister_proxy(saftlib_object_id, received_client_fd, received_signal_group_fd);
				}
				break;
				case 2: // void quit()
					// Loop::get_default().quit_in(std::chrono::milliseconds(100));
					// std::cerr << "saftd will quit in 100 ms" << std::endl;
					Loop::get_default().quit();
					std::cerr << "saftd will quit now" << std::endl;
				break;
				case 3: {// bool load_plugin(const std::string &so_filename, const std::string &object_path)
					std::string lib_name;
					// std::string object_path;
					received.get(lib_name);
					// received.get(object_path);
					std::cerr << "loading " << lib_name << std::endl;
					bool plugin_available = false;
					auto plugin = d->plugins.find(lib_name);
					if (plugin != d->plugins.end()) {
						std::cerr << "plugin found" << std::endl;
						plugin_available = true;
					} else {
						auto insertion_result = d->plugins.insert(std::make_pair(lib_name, std::move(std2::make_unique<LibraryLoader>(lib_name))));
						if (insertion_result.second) {
							std::cerr << "plugin inserted" << std::endl;
							plugin = insertion_result.first;
							plugin_available = true;
						} else {
							std::cerr << "plugin insertion failed" << std::endl;
						}
					}
					if (plugin_available) {
						std::string object_path = plugin->second->get_object_path(d->container);
						unsigned object_id = d->container->create_object(object_path, std::move(plugin->second->create_service(d->container)));
						std::cerr << "created new object under object_path " << object_path << " with object_id " << object_id << std::endl;
					}
					send.put(plugin_available);
				}
				break;
				case 4: {// bool remove_service(const std::string &object_path)
					std::string object_path;
					received.get(object_path);
					std::cerr << "remove service at object_path " << object_path << std::endl;
					bool remove_result = d->container->remove_object(object_path);
					send.put(remove_result);
				}
				break;
				case 5: { // get_status() // report all info about server and services
					std::cerr << "get_stats() called sending " << d->container->d->object_path_lookup_table.size() << " objects " << std::endl;
					send.put(d->container->d->objects.size());
					for (auto &object: d->container->d->objects) {
						auto &object_id = object.first;
						auto &service   = object.second;
						send.put(object_id);
						send.put(service->d->object_path);
						send.put(service->d->interface_names);
						send.put(service->d->signal_fds_use_count);
						send.put(service->d->owner);
					}
					auto client_info = d->container->d->connection->get_client_info();
					send.put(client_info.size());
					for (auto &client: client_info) {
						send.put(client.process_id);
						send.put(client.client_fd);
						send.put(client.signal_fds);
					}
				}
				break;
				default:
					assert(false);
			}
		} else {
			assert(false);
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
		d->connection = connection;
		// create a Service that allows access to Container functionality
		auto container_service = std2::make_unique<Container_Service>(this);
		unsigned object_id = create_object("/saftbus", std::move(container_service));
		assert(object_id == 1); // the entier system relies on having CoreService at object_id 1	
	}

	Container::~Container() = default;

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
		std::cerr << "remove_object " << object_path << " now" << std::endl;
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result == d->object_path_lookup_table.end()) {
			// we have already registered an object under this object path
			std::cerr << "remove_object: object path " << object_path << " not found " << std::endl;
			return false;
		}
		auto object_id = find_result->second;
		d->objects.erase(object_id);
		d->object_path_lookup_table.erase(object_path);
		return true;
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
		service->call(client_fd, received, send);
		return true;
	}

	void Container::remove_signal_fd(int fd)
	{
		for(auto &service: d->objects) {
			std::cerr << "remove_signal_fd(" << fd << ") for object " << service.second->d->object_path << std::endl;
			service.second->d->remove_signal_fd(fd);
		}
	}

	void Container::clear() {
		d->objects.clear();
		d->object_path_lookup_table.clear();
	}




}
