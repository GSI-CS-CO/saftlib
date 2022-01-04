#include "service.hpp"
#include "make_unique.hpp"
#include "loop.hpp"

#include <string>
#include <map>
#include <set>
#include <cassert>

#include <unistd.h>

namespace mini_saftlib {

	struct Service::Impl {
		int          owner;
		std::set<int> signal_fds;
		std::map<int, int> use_count;
		std::vector<std::string> interface_names;
		unsigned object_id;
	};

	Service::Service(const std::vector<std::string> &interface_names) 
		: d(std2::make_unique<Impl>())
	{
		d->owner = -1;
		d->interface_names = interface_names;
	}
	Service::~Service() = default;

	void Service::call(int client_fd, Deserializer &received, Serializer &send) {
		int interface_no, function_no;
		received.get(interface_no);
		received.get(function_no);
		std::cerr << "Service::call got " << interface_no << " " << function_no << std::endl;
		call(interface_no, function_no, client_fd, received, send);
	}

	void Service::remove_signal_fd(int fd)
	{
		std::cerr << "Service::remove_signal_fd " << fd << std::endl;
		auto found = d->signal_fds.find(fd);
		assert(found != d->signal_fds.end());
		auto found_use_count = d->use_count.find(fd);
		assert(found_use_count != d->use_count.end());
		d->use_count.erase(fd);
		d->signal_fds.erase(fd);
	}

	void Service::emit(Serializer &send)
	{
		std::cerr << "emitting signal" << std::endl;
		for (auto &fd: d->signal_fds) {
			std::cerr << "   to " << fd << std::endl;
			send.write_to_no_init(fd);
		}
		send.put_init();
	}

	int Service::get_object_id() 
	{
		return d->object_id;
	}

	struct ContainerService::Impl {
		ServiceContainer *container;
		Serializer serialized_signal;
		static std::vector<std::string> gen_interface_names();
	};


	std::vector<std::string> ContainerService::Impl::gen_interface_names() {
		std::vector<std::string> interface_names;
		interface_names.push_back("de.gsi.saftlib");
		return interface_names;
	}

	ContainerService::ContainerService(ServiceContainer *container)
		: Service(Impl::gen_interface_names())
		, d(std2::make_unique<Impl>())
	{
		d->container = container;
		Loop::get_default().connect(
			std::move(std2::make_unique<mini_saftlib::TimeoutSource>(sigc::mem_fun(this,&ContainerService::emit_periodical_signal),std::chrono::milliseconds(1000)
				) 
			)
		);

	}
	ContainerService::~ContainerService() = default;

	bool ContainerService::emit_periodical_signal() {
		static int count = 0;
		d->serialized_signal.put(get_object_id());
		int interface_no = 0;
		d->serialized_signal.put(interface_no);
		d->serialized_signal.put(count++);
		std::cerr << "emit signal with counter value " << count << std::endl;
		emit(d->serialized_signal);
		return true;
	}

	void ContainerService::call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) {
		std::cerr << "ContainerService called" << std::endl;
		if (interface_no == 0) {
			switch(function_no) {
				case 0: {// register proxy
					std::cerr << "register_proxy called" << std::endl;
					std::string object_path;
					received.get(object_path);
					int signal_fd;
					received.get(signal_fd);
					if (signal_fd == -1) {
						signal_fd = recvfd(client_fd);
						std::cerr << "got (open) " << signal_fd << std::endl;
					} else {
						std::cerr << "reuse " << signal_fd << std::endl;
					}
					unsigned saftlib_object_id = d->container->register_proxy(object_path, client_fd, signal_fd);
					std::cerr << "registered proxy for saftlib_object_id " << saftlib_object_id << std::endl;
					send.put(saftlib_object_id);
					send.put(client_fd); // fd and signal_fd are used in the proxy de-registration process
					send.put(signal_fd);
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
					bool result = true;
					send.put(result);
				}
				break;
				case 2: // quit
					Loop::get_default().quit_in(std::chrono::milliseconds(100));
					std::cerr << "saftd will quit in 100 ms" << std::endl;
				break;
				case 3: {// greet
					std::string name;
					received.get(name);
					std::cerr << "hello " << name << " from ContainerService" << std::endl;
				}
				break;
				default:
					assert(false);
			}
		} else {
			assert(false);
		}
	}






	struct ServiceContainer::Impl {
		unsigned generate_saftlib_object_id();
		ServerConnection *connection;

		struct Object {
			std::unique_ptr<Service> service;
			std::string object_path;
			// std::set<int> client_fds;
		};

		std::map<unsigned, std::unique_ptr<Object> > objects;
		std::map<std::string, unsigned> object_path_lookup_table; // maps object_path to saftlib_object_id
	};
	// generate a unique object_id != 0
	unsigned ServiceContainer::Impl::generate_saftlib_object_id() {
		static unsigned saftlib_object_id_generator = 1;
		while ((objects.find(saftlib_object_id_generator) != objects.end()) ||
		        saftlib_object_id_generator == 0) {
			++saftlib_object_id_generator;
		}
		return saftlib_object_id_generator++;
	}

	ServiceContainer::ServiceContainer(ServerConnection *connection) 
		: d(std2::make_unique<Impl>())
	{
		d->connection = connection;
		// create a Service that allows access to ServiceContainer functionality
		auto container_service = std2::make_unique<ContainerService>(this);
		unsigned object_id = create_object("/de/gsi/saftlib", std::move(container_service));
		assert(object_id == 1); // the entier system relies on having CoreService at object_id 1	
	}

	ServiceContainer::~ServiceContainer() = default;

	unsigned ServiceContainer::create_object(const std::string &object_path, std::unique_ptr<Service> service)
	{
		unsigned saftlib_object_id = d->generate_saftlib_object_id();
		service->d->object_id = saftlib_object_id;
		auto insertion_result = d->objects.insert(std::make_pair(saftlib_object_id, std2::make_unique<Impl::Object>()));
		// insertion_result is a pair, where the 'first' member is an iterator to the inserted element,
		// and the 'second' member is a bool that is true if the insertion took place and false if the insertion failed.
		auto  insertion_took_place  = insertion_result.second;
		auto &inserted_object       = insertion_result.first->second; 
		if (insertion_took_place) {
			inserted_object->service     = std::move(service);
			inserted_object->object_path = object_path;
			d->object_path_lookup_table[object_path] = saftlib_object_id;
			return saftlib_object_id;
		}
		return 0;
	}

	unsigned ServiceContainer::register_proxy(const std::string &object_path, int client_fd, int signal_group_fd)
	{
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result != d->object_path_lookup_table.end()) {
			unsigned saftlib_object_id = find_result->second;
			auto find_result = d->objects.find(saftlib_object_id);
			assert(find_result != d->objects.end()); // if this cannot be found, the lookup table is not correct
			auto    &object    = find_result->second;
			object->service->d->signal_fds.insert(signal_group_fd);
			object->service->d->use_count[signal_group_fd]++;
			std::cerr << "register_proxy: object use count = " << object->service->d->use_count[signal_group_fd] << std::endl;
			d->connection->register_signal_id_for_client(client_fd, signal_group_fd);
			return saftlib_object_id;
		}
		return 0;
	}
	void ServiceContainer::unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd)
	{
		auto find_result = d->objects.find(saftlib_object_id);
		assert(find_result != d->objects.end()); 
		auto    &object    = find_result->second;
		object->service->d->use_count[signal_group_fd]--;
		d->connection->unregister_signal_id_for_client(client_fd, signal_group_fd);
		std::cerr << "unregister_proxy: signal fd " << signal_group_fd << " use count = " << object->service->d->use_count[signal_group_fd] << std::endl;
		if (object->service->d->use_count[signal_group_fd] == 0) {
			object->service->d->signal_fds.erase(signal_group_fd);
		}
	}


	bool ServiceContainer::call_service(unsigned saftlib_object_id, int client_fd, Deserializer &received, Serializer &send) {
		auto result = d->objects.find(saftlib_object_id);
		if (result == d->objects.end()) {
			return false;
		}
		result->second->service->call(client_fd, received, send);
		return true;
	}

	void ServiceContainer::remove_signal_fd(int fd)
	{
		for(auto &object: d->objects) {
			object.second->service->remove_signal_fd(fd);
		}
	}



}
