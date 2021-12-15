
#include <client_connection.hpp>
#include <iostream>

int main()
{
	// mini_saftlib::ClientConnection connection;
	// connection.send_call();

	mini_saftlib::Proxy saftd1("/de/gsi/saftlib", mini_saftlib::SignalGroup::get_global());
	mini_saftlib::Proxy saftd2("/de/gsi/saftlib", mini_saftlib::SignalGroup::get_global());



	return 0;
}