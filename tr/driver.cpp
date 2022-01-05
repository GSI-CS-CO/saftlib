#include "driver.hpp"

#include <iostream>

namespace timingreceiver 
{

	Timingreceiver_Serivce::Timingreceiver_Serivce()
		: mini_saftlib::Service(gen_interface_names())
	{
		std::cerr << "Timingreceiver_Serivce constructor" << std::endl;
	}

	std::vector<std::string> Timingreceiver_Serivce::gen_interface_names()
	{
		std::vector<std::string> result;
		result.push_back("de.gsi.saftlib.TimingReceiver");
		return result;
	}

	void Timingreceiver_Serivce::call(unsigned interface_no, unsigned function_no, int client_fd, mini_saftlib::Deserializer &received, mini_saftlib::Serializer &send)
	{

	}

}

extern "C" std::unique_ptr<mini_saftlib::Service> create_service() {
	return std2::make_unique<timingreceiver::Timingreceiver_Serivce>();
}
