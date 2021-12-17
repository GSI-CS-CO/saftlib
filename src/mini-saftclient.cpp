
#include <client_connection.hpp>
#include <iostream>
#include <thread>

#include <saftd.hpp>

int main(int argc, char **argv)
{
	// mini_saftlib::ClientConnection connection;
	// connection.send_call();

	// mini_saftlib::Proxy saftd1("/de/gsi/saftlib", mini_saftlib::SignalGroup::get_global());
	// mini_saftlib::Proxy saftd2("/de/gsi/saftlib", mini_saftlib::SignalGroup::get_global());

	auto core_service_proxy = mini_saftlib::Proxy::create();
	auto core_service_proxy2= mini_saftlib::Proxy::create();

	std::cerr << "argc = " << argc << std::endl;
	if (argc > 1) {
		core_service_proxy->quit();
	}

	return 0;
}