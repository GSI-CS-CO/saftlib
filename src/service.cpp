#include "service.hpp"

#include <string>

namespace mini_saftlib {

	struct Service::Impl {
		int          owner;
		service_call call;
	};

	Service::Service(std::vector<std::string> interface_names, service_call call) 
		: d(std::make_unique<Impl>())
	{
		d->owner = -1;
		d->call  = call;
	}
	Service::~Service() = default;

}
