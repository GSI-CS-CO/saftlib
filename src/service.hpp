#ifndef MINI_SAFTLIB_SERVICE_
#define MINI_SAFTLIB_SERVICE_

#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	extern "C" {
		typedef void (*service_call)(int interface_no, int fucntion_no, void *object);
	}

	class Service {
		Service(std::vector<std::string> interface_names, service_call call);
		~Service();
	private:
		struct Impl;
		std::unique_ptr<Impl> d;
	};


}

#endif
