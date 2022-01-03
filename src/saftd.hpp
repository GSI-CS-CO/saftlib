#ifndef MINI_SAFTLIB_SAFTD_
#define MINI_SAFTLIB_SAFTD_

#include "client.hpp"
#include "service.hpp"
#include "saftbus.hpp"

#include <cassert>

namespace mini_saftlib {

	// this is boilerplate and should be generated later (code generator is not yet written)
	class SAFTd_Proxy : public Proxy {
	public:
		SAFTd_Proxy(SignalGroup &signal_group = SignalGroup::get_global());
		void greet(const std::string &name);
		bool signal_dispatch(int interface, Deserializer &signal_content);
		bool quit();
		static std::shared_ptr<SAFTd_Proxy> create(SignalGroup &signal_group = SignalGroup::get_global());
	};

	class SAFTd_Driver {
	public:	
		// saftlib export
		void greet(const std::string &name);

		// saftlib export
		bool quit();
	private:
	};


	class SAFTd_Service : public Service {
		static std::vector<std::string> gen_interface_names();
	public:
		SAFTd_Service();
		SAFTd_Driver *impl;
		void call(int interface_no, int function_no, Deserializer &received, Serializer &send);
	};

	// extern "C" {
	// 	void SAFTd_service_call(int interface_no, int function_no, Service *service, Deserializer &received, Serializer &send);
	// }



}


#endif
