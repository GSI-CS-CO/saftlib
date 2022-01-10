#include "client.hpp"
// #include "loop.hpp"
#include "make_unique.hpp"
// #include "timingreceiver_proxy.hpp"

#include <iostream>
#include <thread>

// bool timeout() {
// 	static int i = 0;
// 	std::cerr << "#" << i++ << std::endl;
// 	// auto core_service_proxy = mini_saftlib::ContainerService_Proxy::create();
// 	// mini_saftlib::Loop::get_default().quit();
// 	if (i == 5) {
// 		mini_saftlib::Loop::get_default().quit();
// 	}
// 	return true;
// }

// bool got_signal(int fd, int condition) {
// 	std::cerr << "got_signal" << std::endl;
// 	if (condition & POLLIN) {
// 		mini_saftlib::SignalGroup::get_global().wait_for_signal(0);
// 	}
// 	return true;
// }

int main(int argc, char **argv)
{
	auto core_service_proxy = mini_saftlib::ContainerService_Proxy::create();
	auto sg2 = mini_saftlib::SignalGroup();
	auto core_service_proxy2 = mini_saftlib::ContainerService_Proxy::create(sg2);
	auto core_service_proxy3 = mini_saftlib::ContainerService_Proxy::create(sg2);


	// for(;;) {
	// 	mini_saftlib::SignalGroup::get_global().wait_for_signal();
	// }
	// auto core_service_proxy2= mini_saftlib::ContainerService_Proxy::create();

	// mini_saftlib::Loop::get_default().connect(
	// 	std::move(
	// 		std2::make_unique<mini_saftlib::TimeoutSource>(
	// 			sigc::ptr_fun(timeout),std::chrono::milliseconds(1000)
	// 		) 
	// 	)
	// );

	// mini_saftlib::Loop::get_default().connect(
	// 	std::move(
	// 		std2::make_unique<mini_saftlib::IoSource>(
	// 			sigc::ptr_fun(got_signal),  mini_saftlib::SignalGroup::get_global().get_fd(), POLLIN
	// 		) 
	// 	)
	// );

	// char ch;
	// std::cin >> ch; 

	// std::cerr << "argc = " << argc << std::endl;
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			std::string argvi(argv[i]);
			if (argvi == "-q") {
				core_service_proxy->quit();
			} if (argvi == "-s") {
				mini_saftlib::SaftbusInfo saftbus_info = core_service_proxy->get_status();
				return 0;
			} else if (argvi == "-l") {
				if ((i+=2) < argc) {
					core_service_proxy->load_plugin(argv[i-1], argv[i]);
				} else {
					throw std::runtime_error("expect so-filename and object-path after -l");
				}
			}
		}
	} else {
		// mini_saftlib::Loop::get_default().run();
		for (int i=0; i<5; ++i) {
			mini_saftlib::SignalGroup::get_global().wait_for_signal();
		}
	}


	return 0;
}