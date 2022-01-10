#ifndef TR_DRIVER_HPP_
#define TR_DRIVER_HPP_

#include <service.hpp>
#include <make_unique.hpp>
#include <saftbus.hpp>

#include <etherbone.h>

#include <memory>


namespace timingreceiver {

	class Timingreceiver_Serivce : public mini_saftlib::Service 
	{
	public:
		Timingreceiver_Serivce();
		static std::vector<std::string> gen_interface_names();
		void call(unsigned interface_no, unsigned function_no, int client_fd, mini_saftlib::Deserializer &received, mini_saftlib::Serializer &send);

	    etherbone::Socket socket;

	};

}

extern "C" std::unique_ptr<mini_saftlib::Service> create_service();


#endif
