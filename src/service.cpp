#include "service.hpp"
#include "container.hpp"
#include "make_unique.hpp"
#include "loop.hpp"

#include <string>
#include <cassert>

namespace mini_saftlib {

	struct Service::Impl {
		int          owner;
		std::vector<int> signal_group_fds;
	};

	Service::Service(std::vector<std::string> interface_names) 
		: d(std2::make_unique<Impl>())
	{
		d->owner = -1;
		d->signal_group_fds.reserve(128);
	}
	Service::~Service() = default;

	void Service::add_signal_group(int fd) {
		d->signal_group_fds.push_back(fd);
	}

	void Service::call(Deserializer &received, Serializer &send) {
		int interface_no, function_no;
		received.get(interface_no);
		received.get(function_no);
		std::cerr << "Service::call got " << interface_no << " " << function_no << std::endl;
		call(interface_no, function_no, received, send);
	}

	struct CoreService::Impl {
		Container container;
	};


	std::vector<std::string> CoreService::gen_interface_names() {
		std::vector<std::string> interface_names;
		interface_names.push_back("de.gsi.saftlib");
		return interface_names;
	}

	// CoreService* CoreService::get_instance()
	// {
	// 	static auto instance = std2::make_unique<CoreService>();
	// 	static auto ptr = instance.get();
	// 	instance->create_object("/de/gsi/saftlib", std::move(instance));
	// 	return ptr;
	// }


	CoreService::CoreService()
		: Service(gen_interface_names())
		, d(std2::make_unique<Impl>())
	{
		// std::unique_ptr<Service> service = std2::make_unique<CoreService>();
		// d->container.create_object("/de/gsi/saftlib", std::move(service));
	}
	CoreService::~CoreService() = default;

	// just forward the calls to the underlying container object
	unsigned CoreService::create_object(const std::string &object_path, std::unique_ptr<Service> service)
	{
		return d->container.create_object(object_path, std::move(service));
	}
	unsigned CoreService::register_proxy(const std::string &object_path, int client_fd, int signal_group_fd)
	{
		return d->container.register_proxy(object_path, client_fd, signal_group_fd);
	}
	bool CoreService::call_service(unsigned saftlib_object_id, Deserializer &received, Serializer &send)
	{
		return d->container.call_service(saftlib_object_id, received, send);
	}



	void CoreService::call(unsigned interface_no, unsigned function_no, Deserializer &received, Serializer &send) {
		std::cerr << "CoreService called" << std::endl;
		if (interface_no == 0) {
			switch(function_no) {
				case 0: {// unregister proxy;
					std::cerr << "unregister_proxy called" << std::endl;
					unsigned saftlib_object_id;
					int client_fd, signal_group_fd;
					received.get(saftlib_object_id);
					received.get(client_fd);
					received.get(signal_group_fd);
					d->container.unregister_proxy(saftlib_object_id, client_fd, signal_group_fd);
					bool result;
					send.put(result = true);
				}
				break;
				case 1: // quit
					Loop::get_default().quit_in(std::chrono::milliseconds(100));
					std::cerr << "saftd will quit in 1000 ms" << std::endl;
				break;
				case 2: {// greet
					std::string name;
					received.get(name);
					std::cerr << "hello " << name << " from CoreService" << std::endl;
				}
				break;
				default:
					assert(false);
			}
		} else {
			assert(false);
		}
	}

}
