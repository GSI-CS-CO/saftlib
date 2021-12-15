#include "service.hpp"

#include <string>

namespace mini_saftlib {

	struct Service::Impl {
		int          owner;
		service_call call;

		std::vector<int> signal_group_fds;
	};

	Service::Service(std::vector<std::string> interface_names, service_call call) 
		: d(std::make_unique<Impl>())
	{
		d->owner = -1;
		d->call  = call;
		d->signal_group_fds.reserve(128);
	}
	Service::~Service() = default;

	void Service::add_signal_group(int fd) {
		d->signal_group_fds.push_back(fd);
	}

}
