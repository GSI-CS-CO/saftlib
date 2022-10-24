#ifndef SAFTD_SERVICE_HPP_
#define SAFTD_SERVICE_HPP_

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
	class OpenDevice;

	class SAFTd_Service : public saftbus::Service 
	{
	public:
		SAFTd_Service();
		~SAFTd_Service();
		static std::vector<std::string> gen_interface_names();
		void call(unsigned interface_no, unsigned function_no, int client_fd, saftbus::Deserializer &received, saftbus::Serializer &send);


		std::string AttachDevice(const std::string& name, const std::string& path);
		void RemoveDevice(const std::string& name);

		void msi_handler(eb_data_t msi);

		etherbone::Socket socket;
		// std::unique_ptr<Device> device;

		MSI_Source *msi_source;
		EB_Source  *eb_source;
		eb_socket_t eb_socket;

		std::map< std::string, std::unique_ptr<OpenDevice> > devs;

	};

}

extern "C" std::unique_ptr<saftbus::Service> create_service();


#endif
