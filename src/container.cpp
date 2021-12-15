#include "container.hpp"

#include <map>
#include <string>
#include <set>
#include <cassert>

namespace mini_saftlib {

	struct Container::Impl {
		unsigned generate_saftlib_object_id();

		struct Object {
			Service *service;
			std::string object_path;
			std::set<int> client_fds;
			std::set<int> signal_fds;
		};

		std::map<unsigned, std::unique_ptr<Object> > objects;
		std::map<std::string, unsigned> object_path_lookup_table; // maps object_path to saftlib_object_id
	};
	// generate a unique object_id != 0
	unsigned Container::Impl::generate_saftlib_object_id() {
		static unsigned saftlib_object_id_generator = 0;
		while ((objects.find(saftlib_object_id_generator) != objects.end()) ||
			   (saftlib_object_id_generator == 0)) {
			++saftlib_object_id_generator;
		}
		return saftlib_object_id_generator++;
	}

	Container::Container() 
		: d(std::make_unique<Impl>())
	{
	}

	Container::~Container() = default;

	unsigned Container::create_object(const std::string &object_path, Service *service)
	{
		unsigned saftlib_object_id = d->generate_saftlib_object_id();
		auto insertion_result = d->objects.insert(std::make_pair(saftlib_object_id, std::make_unique<Impl::Object>()));
		// insertion_result is a pair, where the 'first' member is an iterator to the inserted element,
		// and the 'second' member is a bool that is true if the insertion took place and false if the insertion failed.
		auto  insertion_took_place  = insertion_result.second;
		auto &inserted_object       = insertion_result.first->second; 
		if (insertion_took_place) {
			inserted_object->service     = service;
			inserted_object->object_path = object_path;
			d->object_path_lookup_table[object_path] = saftlib_object_id;
			return saftlib_object_id;
		}
		return 0;
	}

	unsigned Container::register_proxy(const std::string &object_path, int client_fd, int signal_group_fd)
	{
		auto find_result = d->object_path_lookup_table.find(object_path);
		if (find_result != d->object_path_lookup_table.end()) {
			unsigned object_id = find_result->second;
			auto find_result = d->objects.find(object_id);
			assert(find_result != d->objects.end()); // if this cannot be found, the lookup table is not correct
			auto    &object    = find_result->second;
			object->client_fds.insert(client_fd);
			object->signal_fds.insert(signal_group_fd);
			return object_id;
		}
		return 0;
	}


	Service *Container::get_service_for_object(unsigned saftlib_object_id)
	{
		auto result = d->objects.find(saftlib_object_id);
		if (result == d->objects.end()) {
			return nullptr;
		}
		return result->second->service;
	}




}