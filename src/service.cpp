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

	void Service::call(int client_fd, Deserializer &received, Serializer &send) {
		int interface_no, function_no;
		received.get(interface_no);
		received.get(function_no);
		std::cerr << "Service::call got " << interface_no << " " << function_no << std::endl;
		call(interface_no, function_no, client_fd, received, send);
	}

	struct CoreService::Impl {
		Container *container;
	};


	std::vector<std::string> CoreService::gen_interface_names() {
		std::vector<std::string> interface_names;
		interface_names.push_back("de.gsi.saftlib");
		return interface_names;
	}

	CoreService::CoreService(Container *container)
		: Service(gen_interface_names())
		, d(std2::make_unique<Impl>())
	{
		d->container = container;
	}
	CoreService::~CoreService() = default;

	void CoreService::call(unsigned interface_no, unsigned function_no, int client_fd, Deserializer &received, Serializer &send) {
		std::cerr << "CoreService called" << std::endl;
		if (interface_no == 0) {
			switch(function_no) {
				case 0: {// register proxy
					std::cerr << "register_proxy called" << std::endl;
					std::string object_path;
					received.get(object_path);
					int signal_fd = recvfd(client_fd);
					unsigned saftlib_object_id = d->container->register_proxy(object_path, client_fd, signal_fd);
					std::cerr << "registered proxy under saftlib_object_id " << saftlib_object_id << std::endl;
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
