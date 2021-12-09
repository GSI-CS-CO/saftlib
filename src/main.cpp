#include <loop.hpp>
#include <iostream>

#include <fcntl.h>  // open()
#include <unistd.h> // read()

bool timeout_callback() {
	static int i = 3;
	std::cout << "tick" << std::endl;
	if (--i == 0) {
		i = 3;
		return false;
	}
	return true;
}

bool timeout_callback2(mini_saftlib::Loop *loop) {
	static int i = 0;
	static int j = 0;
	std::cout << "  tock" << std::endl;
	if (++i == 6) {
		i = 0;
		loop->connect(std::make_shared<mini_saftlib::TimeoutSource>(sigc::ptr_fun(timeout_callback),std::chrono::milliseconds(1000), std::chrono::milliseconds(-500)));
	}
	if (++j == 10) {
		loop->quit();
	}
	return true;
}

bool fd_callback(int fd, int condition) {
	char ch;
	read(fd, &ch, 1);
	std::cerr << ch;
	return ch != 'x';
}

int main() {

	mini_saftlib::Loop loop;

	int fd = open("my_pipe", O_RDONLY | O_NONBLOCK);

	loop.connect(std::make_shared<mini_saftlib::IoSource>(sigc::ptr_fun(fd_callback), fd, POLLIN));
	loop.connect(std::make_shared<mini_saftlib::TimeoutSource>(sigc::ptr_fun(timeout_callback ), std::chrono::milliseconds(1000)));
	loop.connect(std::make_shared<mini_saftlib::TimeoutSource>(sigc::bind(sigc::ptr_fun(timeout_callback2),&loop), std::chrono::milliseconds(1000), std::chrono::milliseconds(500)));
	loop.run();

	return 0;
}