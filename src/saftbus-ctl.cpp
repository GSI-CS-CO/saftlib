#include "client.hpp"
// #include "loop.hpp"
// #include "make_unique.hpp"
// #include "timingreceiver_proxy.hpp"

#include <iostream>
#include <thread>

// #include <poll.h>

// bool timeout() {
// 	static int i = 0;
// 	std::cerr << "#" << i++ << std::endl;
// 	// auto core_service_proxy = saftbus::Container_Proxy::create();
// 	// saftbus::Loop::get_default().quit();
// 	if (i == 5) {
// 		saftbus::Loop::get_default().quit();
// 	}
// 	return true;
// }

// bool got_signal(int fd, int condition) {
// 	std::cerr << "got_signal" << std::endl;
// 	if (condition & POLLIN) {
// 		saftbus::SignalGroup::get_global().wait_for_signal(0);
// 	}
// 	return true;
// }

int main(int argc, char **argv)
{
	auto core_service_proxy = saftbus::Container_Proxy::create();
	// auto sg2 = saftbus::SignalGroup();
	// auto core_service_proxy2 = saftbus::Container_Proxy::create(sg2);
	// auto core_service_proxy3 = saftbus::Container_Proxy::create(sg2);


	// for(;;) {
	// 	saftbus::SignalGroup::get_global().wait_for_signal();
	// }
	// auto core_service_proxy2= saftbus::Container_Proxy::create();

	// saftbus::Loop::get_default().connect(
	// 	std::move(
	// 		std2::make_unique<saftbus::TimeoutSource>(
	// 			sigc::ptr_fun(timeout),std::chrono::milliseconds(1000)
	// 		) 
	// 	)
	// );

	// saftbus::Loop::get_default();
	// saftbus::Loop::get_default().connect<saftbus::IoSource>(
	// 			&got_signal,  saftbus::SignalGroup::get_global().get_fd(), POLLIN
	// );

	// char ch;
	// std::cin >> ch; 

	// std::cerr << "argc = " << argc << std::endl;
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
		// saftbus::Loop::get_default().run();
		for (int i=0; i<50; ++i) {
			saftbus::SignalGroup::get_global().wait_for_signal();
		}
	}

	return 0;
}