#include "client.hpp"
#include "make_unique.hpp"
#include "timingreceiver_proxy.hpp"

#include <iostream>
#include <thread>

int main(int argc, char **argv)
{
	// auto core_service_proxy = mini_saftlib::ContainerService_Proxy::create();
	// auto core_service_proxy2 = mini_saftlib::ContainerService_Proxy::create();
	// auto core_service_proxy3 = mini_saftlib::ContainerService_Proxy::create();
	auto timingreceiver_proxy = mini_saftlib::TimingReceiver_Proxy::create("/de/gsi/saftlib/tr0");

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
	// if (argc > 1) {
	// 	core_service_proxy->quit();
	// } else {
		std::cerr << std::hex << std::setw(8) << std::setfill('0') << timingreceiver_proxy->eb_read(0x20140000) << std::endl;
		// for (int i = 0; i < 5; ++i ) {
		// 	mini_saftlib::SignalGroup::get_global().wait_for_signal();
		// }
	// }


	return 0;
}