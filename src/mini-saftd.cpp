#include <loop.hpp>
#include <server_connection.hpp>
#include <service.hpp>
#include <loop.hpp>
#include <make_unique.hpp>
#include <container.hpp>

#include <iostream>
#include <memory>


#include <fcntl.h>  // open()
#include <unistd.h> // read()

bool timeout_tick() {
	std::cout << "tick" << std::endl;
	static int i = 0;
	return (++i==3)?(i=0):i;
}

bool timeout_tock() {
	std::cout << "  tock" << std::endl;
	static int i = 0;
	// static int j = 0;
	if (++i == 6) {
		i = 0;
		mini_saftlib::Loop::get_default().connect(
			std::move(
				std2::make_unique<mini_saftlib::TimeoutSource>(
					sigc::ptr_fun(timeout_tick),std::chrono::milliseconds(1000), std::chrono::milliseconds(-500)
				)
			)
		);
	}
	return true;
}

static bool fd_callback(int fd, int condition);
void init_fd(){ 
	std::cerr << "init_fd" << std::endl;
	int fd = open("my_pipe", O_RDONLY | O_NONBLOCK);
	mini_saftlib::Loop::get_default().connect(
		std::move(
			std2::make_unique<mini_saftlib::IoSource>(
				sigc::ptr_fun(fd_callback), fd, POLLIN | POLLHUP
			)
		)
	);
}
bool fd_callback(int fd, int condition) {
	if (condition & POLLHUP) {
		std::cerr << "pollhup called" << std::endl;
		close(fd);
		init_fd();
		return false;
	}
	const int size = 10;
	char ch[size] = {0,};
	read(fd, &ch, size-1);
	std::cerr << ch;
	return true;
}

int main() {
	// two lines just to play around ... has nothing to do with the saftd functionality.
	init_fd();
	mini_saftlib::Loop::get_default().connect(std::move(std2::make_unique<mini_saftlib::TimeoutSource>(sigc::ptr_fun(timeout_tock), std::chrono::milliseconds(1000), std::chrono::milliseconds(500))));

	// system setup:
	// 1) Creat a container for services
	mini_saftlib::Container container;
	// 2) Create one core_service object
	// auto core_service = std2::make_unique<mini_saftlib::CoreService>(&container);
	// 3) Create the server connection that instantiates a socket and manages incoming 
	//    client requests. Client request are redirected to the CoreService object, therefore
	//    a pointer to the core_service object is passed to the ServerConnection constructor.
	mini_saftlib::ServerConnection server_connection(&container);
	// 4) insert the core_service object into the container
	// container.create_object("/de/gsi/saftlib", std::move(core_service));	
	// 5) run the main loop
	mini_saftlib::Loop::get_default().run();

	return 0;
}