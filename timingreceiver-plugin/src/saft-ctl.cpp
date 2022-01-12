#include "client.hpp"
#include "make_unique.hpp"
#include "SAFTd_proxy.hpp"

#include <iostream>
#include <thread>

int main(int argc, char **argv)
{
	// auto core_service_proxy = saftbus::ContainerService_Proxy::create();
	// auto core_service_proxy2 = saftbus::ContainerService_Proxy::create();
	// auto core_service_proxy3 = saftbus::ContainerService_Proxy::create();
	auto saftd_proxy = saftlib::SAFTd_Proxy::create("/de/gsi/saftlib");

	// for(;;) {
	// 	saftbus::SignalGroup::get_global().wait_for_signal();
	// }
	// auto core_service_proxy2= saftbus::ContainerService_Proxy::create();

	// saftbus::Loop::get_default().connect(
	// 	std::move(
	// 		std2::make_unique<saftbus::TimeoutSource>(
	// 			sigc::ptr_fun(timeout),std::chrono::milliseconds(1000)
	// 		) 
	// 	)
	// );

	// saftbus::Loop::get_default().connect(
	// 	std::move(
	// 		std2::make_unique<saftbus::IoSource>(
	// 			sigc::ptr_fun(got_signal),  saftbus::SignalGroup::get_global().get_fd(), POLLIN
	// 		) 
	// 	)
	// );

	// char ch;
	// std::cin >> ch; 

	// std::cerr << "argc = " << argc << std::endl;
	// if (argc > 1) {
	// 	core_service_proxy->quit();
	// } else {
		// for (int i = 0; i < 5; ++i ) {
		// 	saftbus::SignalGroup::get_global().wait_for_signal();
		// }
	// }

	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			std::string argvi(argv[i]);
			if (argvi == "-a") {
				if ((i+=2) < argc) {
					std::cerr << saftd_proxy->AttachDevice(argv[i-1], argv[i]) << std::endl;
				} else {
					throw std::runtime_error("expect name device after -a");
				}
			} else if (argvi == "-r") {
				if ((i+=1) < argc) {
					saftd_proxy->RemoveDevice(argv[i]);
				} else {
					throw std::runtime_error("expect name -r");
				}
			}
		}
	} else {
		std::cerr << std::hex << std::setw(8) << std::setfill('0') << saftd_proxy->eb_read(0x20140000) << std::endl;
	}	


	return 0;
}