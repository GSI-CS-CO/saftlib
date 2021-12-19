
#include "client.hpp"
#include "loop.hpp"
#include "make_unique.hpp"

#include <iostream>
#include <thread>

#include <saftd.hpp>

bool timeout() {
	static int i = 0;
	std::cerr << "#" << i++ << std::endl;
	auto core_service_proxy = mini_saftlib::Proxy::create();
	return true;
}

int main(int argc, char **argv)
{
	auto core_service_proxy = mini_saftlib::Proxy::create();
	auto core_service_proxy2= mini_saftlib::Proxy::create();

	mini_saftlib::Loop::get_default().connect(
		std::move(
			std2::make_unique<mini_saftlib::TimeoutSource>(
				sigc::ptr_fun(timeout),std::chrono::milliseconds(5)
			) 
		)
	);
	mini_saftlib::Loop::get_default().run();

	// std::cerr << "argc = " << argc << std::endl;
	// if (argc > 1) {
	// 	core_service_proxy->quit();
	// }

	return 0;
}