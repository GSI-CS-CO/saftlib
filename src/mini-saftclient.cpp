
#include "client_connection.hpp"

#include <iostream>
#include <thread>

#include <saftd.hpp>

int main(int argc, char **argv)
{
	auto core_service_proxy = mini_saftlib::Proxy::create();
	auto core_service_proxy2= mini_saftlib::Proxy::create();

	std::cerr << "argc = " << argc << std::endl;
	if (argc > 1) {
		core_service_proxy->quit();
	}

	return 0;
}