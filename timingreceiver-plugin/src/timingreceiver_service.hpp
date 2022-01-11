#ifndef TR_DRIVER_HPP_
#define TR_DRIVER_HPP_

#include <service.hpp>
#include <make_unique.hpp>
#include <saftbus.hpp>

#define ETHERBONE_THROWS 1
#include <etherbone.h>

#include <memory>


namespace saftlib {

	class Device;
	class MSI_Source;
	class EB_Source;
	class SAFTd_Service : public saftbus::Service 
	{
	public:
		SAFTd_Service();
		~SAFTd_Service();
		static std::vector<std::string> gen_interface_names();
		void call(unsigned interface_no, unsigned function_no, int client_fd, saftbus::Deserializer &received, saftbus::Serializer &send);

		void msi_handler(eb_data_t msi);

	    etherbone::Socket socket;
	    std::unique_ptr<Device> device;

	    MSI_Source *msi_source;
	    EB_Source  *eb_source;
	    eb_socket_t eb_socket;
	};

}

extern "C" std::unique_ptr<saftbus::Service> create_service();


#endif
