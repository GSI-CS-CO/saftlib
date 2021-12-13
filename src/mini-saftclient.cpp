
#include <client_connection.hpp>
#include <iostream>

int main()
{
	mini_saftlib::ClientConnection connection;
	connection.send_call();
	connection.send_disconnect();


	int ch;
	std::cin >> ch;


	return 0;
}