
#include "client.hpp"
#include "loop.hpp"
#include "make_unique.hpp"

#include <iostream>
#include <thread>

#include <saftd.hpp>

bool timeout() {
	static int i = 0;
	std::cerr << "#" << i++ << std::endl;
	// auto core_service_proxy = mini_saftlib::ContainerService_Proxy::create();
	// mini_saftlib::Loop::get_default().quit();
	if (i == 2) {
		mini_saftlib::Loop::get_default().quit();
	}
	return true;
}

bool got_signal(int fd, int condition) {
	std::cerr << "got_signal" << std::endl;
	if (condition & POLLIN) {
		mini_saftlib::SignalGroup::get_global().wait_for_signal(0);
	}
	return true;
}

int main(int argc, char **argv)
{
	auto core_service_proxy = mini_saftlib::ContainerService_Proxy::create();
	auto core_service_proxy2 = mini_saftlib::ContainerService_Proxy::create();
	auto core_service_proxy3 = mini_saftlib::ContainerService_Proxy::create();
	// for(;;) {
	// 	mini_saftlib::SignalGroup::get_global().wait_for_signal();
	// }
	// auto core_service_proxy2= mini_saftlib::ContainerService_Proxy::create();

	mini_saftlib::Loop::get_default().connect(
		std::move(
			std2::make_unique<mini_saftlib::TimeoutSource>(
				sigc::ptr_fun(timeout),std::chrono::milliseconds(1000)
			) 
		)
	);

	mini_saftlib::Loop::get_default().connect(
		std::move(
			std2::make_unique<mini_saftlib::IoSource>(
				sigc::ptr_fun(got_signal),  mini_saftlib::SignalGroup::get_global().get_fd(), POLLIN
			) 
		)
	);

	char ch;
	std::cin >> ch; 

	// std::cerr << "argc = " << argc << std::endl;
	if (argc > 1) {
		core_service_proxy->quit();
	} else {
		mini_saftlib::Loop::get_default().run();
	}


	return 0;
}