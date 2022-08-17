#include "loop.hpp"
#include "server.hpp"
#include "service.hpp"
#include "plugins.hpp"
#include "make_unique.hpp"

#include "chunck_allocator_rt.hpp"

#include <iostream>
#include <memory>


#include <fcntl.h>  // open()
#include <unistd.h> // read()




saftbus::ChunckAllocatorRT<64,128> allocator128;
saftbus::ChunckAllocatorRT<16,1024> allocator1024;
int heap_allocations = 0;

void *operator new(std::size_t n) {
	// std::cerr << "my operator new" << std::endl;
	// allocator128.print_size();
	// allocator1024.print_size();
	// std::cerr << "heap: " << heap_allocations << std::endl;
	if (n <= 128 && !allocator128.full()) {
		// std::cerr << "small" << std::endl;
		return allocator128.malloc(n);
	} else if (n <= 1024 && !allocator1024.full()) {
		// std::cerr << "big" << std::endl;
		return allocator1024.malloc(n);
	} else {
		++heap_allocations;
		std::cerr << "heap****************************************************" << std::endl;
		return malloc(n);
	}
}

void operator delete(void *p) {
	// std::cerr << "my operator delete" << std::endl;
	// allocator128.print_size();
	// allocator1024.print_size();
	// std::cerr << "heap: " << heap_allocations << std::endl;

	char* ptr = reinterpret_cast<char*>(p);

	if (allocator128.contains(ptr)) {
		// std::cerr << "small" << std::endl;
		allocator128.free(ptr);
	} else if (allocator1024.contains(ptr)) {
		// std::cerr << "big" << std::endl;
		allocator1024.free(ptr);
	} else {
		--heap_allocations;
		// std::cerr << "heap*********************************************************" << std::endl;
		free(p);
	}
}




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
		saftbus::Loop::get_default().connect(
			std::move(
				std2::make_unique<saftbus::TimeoutSource>(
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
	saftbus::Loop::get_default().connect(
		std::move(
			std2::make_unique<saftbus::IoSource>(
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

//	saftbus::LibraryLoader timingreciever_plugin("/home/michael/local/lib/libtiming-receiver.la");


	// two lines just to play around ... has nothing to do with the saftd functionality.
	init_fd();
	saftbus::Loop::get_default().connect(std::move(std2::make_unique<saftbus::TimeoutSource>(sigc::ptr_fun(timeout_tock), std::chrono::milliseconds(1000), std::chrono::milliseconds(500))));

	// create a mini-saftlib-server and let it run
	saftbus::ServerConnection server_connection;

//	server_connection.get_service_container().create_object("/de/gsi/saftlib/tr0", std::move(timingreciever_plugin.create_service()));
	saftbus::Loop::get_default().run();

	// destroy resources before the plugins get unloaded and the destructors arent available anymore
	saftbus::Loop::get_default().clear();
	server_connection.clear();

	return 0;
}