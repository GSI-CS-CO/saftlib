#include "container.hpp"
#include "make_unique.hpp"

#include <map>
#include <string>
#include <set>
#include <cassert>

namespace mini_saftlib {

	struct Container::Impl {
		unsigned generate_saftlib_object_id();

		struct Object {
			std::unique_ptr<Service> service;
			std::string object_path;
			// std::set<int> client_fds;
			std::set<int> signal_fds;
			std::map<int, int> use_count;
		};

		std::map<unsigned, std::unique_ptr<Object> > objects;
		std::map<std::string, unsigned> object_path_lookup_table; // maps object_path to saftlib_object_id
	};
	// generate a unique object_id != 0
	unsigned Container::Impl::generate_saftlib_object_id() {
		static unsigned saftlib_object_id_generator = 1;
		while ((objects.find(saftlib_object_id_generator) != objects.end()) ||
		        saftlib_object_id_generator == 0) {
			++saftlib_object_id_generator;
		}
		return saftlib_object_id_generator++;
	}

	Container::Container() 
		: d(std2::make_unique<Impl>())
	{
		auto core_service = std2::make_unique<CoreService>(this);
		unsigned object_id = create_object("/de/gsi/saftlib", std::move(core_service));
		assert(object_id == 1); // the entier system relies on having CoreService at object_id 1	
	}

	Container::~Container() = default;

	unsigned Container::create_object(const std::string &object_path, std::unique_ptr<Service> service)
	{
		unsigned saftlib_object_id = d->generate_saftlib_object_id();
		auto insertion_result = d->objects.insert(std::make_pair(saftlib_object_id, std2::make_unique<Impl::Object>()));
		// insertion_result is a pair, where the 'first' member is an iterator to the inserted element,
		// and the 'second' member is a bool that is true if the insertion took place and false if the insertion failed.
		auto  insertion_took_place  = insertion_result.second;
		auto &inserted_object       = insertion_result.first->second; 
		if (insertion_took_place) {
			inserted_object->service     = std::move(service);
			inserted_object->object_path = object_path;
			d->object_path_lookup_table[object_path] = saftlib_object_id;
			std::cerr << "created object under object_id " << saftlib_object_id << std::endl;
			return saftlib_object_id;
		}
		return 0;
	}

	unsigned Container::register_proxy(const std::string &object_path, int client_fd, int signal_group_fd)
	{
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result != d->object_path_lookup_table.end()) {
			unsigned saftlib_object_id = find_result->second;
			auto find_result = d->objects.find(saftlib_object_id);
			assert(find_result != d->objects.end()); // if this cannot be found, the lookup table is not correct
			auto    &object    = find_result->second;
			// object->client_fds.insert(client_fd);
			object->signal_fds.insert(signal_group_fd);
			object->use_count[signal_group_fd]++;
			return saftlib_object_id;
		}
		return 0;
	}
	void Container::unregister_proxy(unsigned saftlib_object_id, int client_fd, int signal_group_fd)
	{
		auto find_result = d->objects.find(saftlib_object_id);
		assert(find_result != d->objects.end()); 
		auto    &object    = find_result->second;
		object->use_count[signal_group_fd]--;
		if (object->use_count[signal_group_fd] == 0) {
			// object->client_fds.erase(client_fd);
			object->signal_fds.erase(signal_group_fd);
		}
	}


	bool Container::call_service(unsigned saftlib_object_id, int client_fd, Deserializer &received, Serializer &send) {
		auto result = d->objects.find(saftlib_object_id);
		if (result == d->objects.end()) {
			return false;
		}
		result->second->service->call(client_fd, received, send);
		return true;
	}




}