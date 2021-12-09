#include <loop.hpp>
#include <iostream>

#include <fcntl.h>  // open()
#include <unistd.h> // read()

bool timeout_callback() {
	static int i = 3;
	std::cout << "tick" << std::endl;
	return --i;
}

bool timeout_callback2() {
	std::cout << "tock" << std::endl;
	return true;
}

bool fd_callback(int fd, int condition) {
	char ch;
	read(fd, &ch, 1);
	std::cerr << ch;
	return true;
}

int main() {

	mini_saftlib::Loop loop;

	int fd = open("my_pipe", O_RDONLY | O_NONBLOCK);

	loop.connect(std::make_shared<mini_saftlib::IoSource>(sigc::ptr_fun(fd_callback), fd, POLLIN));
	loop.connect(std::make_shared<mini_saftlib::TimeoutSource>(sigc::ptr_fun(timeout_callback ), std::chrono::milliseconds(1000)));
	loop.connect(std::make_shared<mini_saftlib::TimeoutSource>(sigc::ptr_fun(timeout_callback2), std::chrono::milliseconds(1000), std::chrono::milliseconds(500)));
	loop.run();

	return 0;
}