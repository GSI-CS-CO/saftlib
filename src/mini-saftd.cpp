#include <loop.hpp>
#include <server_connection.hpp>

#include <iostream>

#include <fcntl.h>  // open()
#include <unistd.h> // read()

bool timeout_tick() {
	static int i = 3;
	std::cout << "tick" << std::endl;
	if (--i == 0) {
		i = 3;
		return false;
	}
	return true;
}

bool timeout_tock(mini_saftlib::Loop *loop) {
	static int i = 0;
	// static int j = 0;
	std::cout << "  tock" << std::endl;
	if (++i == 6) {
		i = 0;
		loop->connect(std::make_shared<mini_saftlib::TimeoutSource>(sigc::ptr_fun(timeout_tick),std::chrono::milliseconds(1000), std::chrono::milliseconds(-500)));
	}
	return true;
}

static bool fd_callback(int fd, int condition, mini_saftlib::Loop *loop);

void init_fd(mini_saftlib::Loop *loop){ 
	std::cerr << "init_fd" << std::endl;
	int fd = open("my_pipe", O_RDONLY | O_NONBLOCK);
	loop->connect(std::make_shared<mini_saftlib::IoSource>(sigc::bind(sigc::ptr_fun(fd_callback), loop), fd, POLLIN | POLLHUP));
}
bool fd_callback(int fd, int condition, mini_saftlib::Loop *loop) {
	if (condition & POLLHUP) {
		std::cerr << "pollhup called" << std::endl;
		close(fd);
		init_fd(loop);
		return false;
	}
	char ch;
	read(fd, &ch, 1);
	std::cerr << ch;
	return true;
}

int main() {

	mini_saftlib::Loop loop;

	mini_saftlib::ServerConnection server_connection(loop);

	init_fd(&loop);

	loop.connect(std::make_shared<mini_saftlib::TimeoutSource>(sigc::bind(sigc::ptr_fun(timeout_tock),&loop), std::chrono::milliseconds(1000), std::chrono::milliseconds(500)));
	loop.run();

	return 0;
}