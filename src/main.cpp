#include <loop.hpp>
#include <iostream>

bool timeout_callback() {
	std::cout << "tick" << std::endl;
	return true;
}

bool timeout_callback2() {
	std::cout << "tock" << std::endl;
	return true;
}

int main() {

	mini_saftlib::Loop loop;
	loop.connect_timeout(sigc::ptr_fun<bool>(timeout_callback ), std::chrono::milliseconds(1000));
	loop.connect_timeout(sigc::ptr_fun<bool>(timeout_callback2), std::chrono::milliseconds(1000), std::chrono::milliseconds(-500));
	loop.run();


	return 0;
}