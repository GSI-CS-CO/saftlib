#ifndef MINI_SAFTLIB_SERVICE_
#define MINI_SAFTLIB_SERVICE_

#include <memory>
#include <string>
#include <vector>

namespace mini_saftlib {

	extern "C" {
		typedef void (*service_call)(int interface_no, int function_no, void *object);
	}

	class Service {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		Service(std::vector<std::string> interface_names, service_call call);
		~Service();
		void add_signal_group(int fd);
	};


}

#endif
