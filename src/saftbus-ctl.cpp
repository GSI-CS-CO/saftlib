#include "client.hpp"

#include <iostream>
#include <thread>

int main(int argc, char **argv)
{
	auto core_service_proxy = saftbus::Container_Proxy::create();

	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			std::string argvi(argv[i]);
			if (argvi == "-q") {
				std::cerr << "call proxy->quit()" << std::endl;
				core_service_proxy->quit();
				std::cerr << "quit done" << std::endl;
			} if (argvi == "-s") {
				saftbus::SaftbusInfo saftbus_info = core_service_proxy->get_status();
				return 0;
			} else if (argvi == "-l") {
				if ((i+=1) < argc) {
					core_service_proxy->load_plugin(argv[i]);
				} else {
					throw std::runtime_error("expect la-filename after -l");
				}
			} else if (argvi == "-r") {
				if ((i+=1) < argc) {
					core_service_proxy->remove_object(argv[i]);
				} else {
					throw std::runtime_error("expect object_path -r");
				}
			}
		}
	} else {
		
		for (int i=0; i<10; ++i) {
			saftbus::SignalGroup::get_global().wait_for_signal();
		}
	}

	return 0;
}