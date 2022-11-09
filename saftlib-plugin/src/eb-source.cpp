#include "eb-source.hpp"

#include <chrono>
#include <algorithm>
#include <array>
#include <thread>
#include <iostream>
#include <cstring>
#include <cassert>

namespace saftlib {


	EB_Source::EB_Source(etherbone::Socket socket_)
	 : Source(), socket(socket_)
	{
		fds.reserve(8);
		fds_it_valid = false;
	}

	EB_Source::~EB_Source()
	{
		std::cerr << "~EB_Source()" << std::endl;
	}

	int EB_Source::add_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		EB_Source* self = (EB_Source*)data;

		self->fds.push_back(pollfd());
		pollfd &pfd = self->fds.back();
		pfd.fd = fd;
		pfd.events = POLLERR | POLLHUP;
		pfd.revents = 0;

		if ((mode & EB_DESCRIPTOR_IN)  != 0) pfd.events |= POLLIN;
		if ((mode & EB_DESCRIPTOR_OUT) != 0) pfd.events |= POLLOUT;

		self->add_poll(&pfd);

		return 0;
	}

	int EB_Source::get_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		EB_Source* self = (EB_Source*)data;

		// check if iterator is valid and (by chance or clever prediction) points to the correct element in the vector<pollfd> fds
		if (!self->fds_it_valid || self->fds_it->fd != fd) {
			for (self->fds_it = self->fds.begin(); self->fds_it != self->fds.end(); ++self->fds_it) {
				if (self->fds_it->fd == fd) {
					break;
				}
			}
			self->fds_it_valid = true; // even if no fd matches, the iterator is valid after the search procedure
		}

		if (self->fds_it == self->fds.end()) {
			return 0;
		}

		int flags = self->fds_it->revents;
		++self->fds_it; // expect the next call to this function will be with the fd of the proceeding element in vector<pollfd> fds
		return 
			((mode & EB_DESCRIPTOR_IN)  != 0 && (flags & (POLLIN  | POLLERR | POLLHUP)) != 0) ||
			((mode & EB_DESCRIPTOR_OUT) != 0 && (flags & (POLLOUT | POLLERR | POLLHUP)) != 0);
	}

	static int no_fd(eb_user_data_t data, eb_descriptor_t fd, uint8_t mode)
	{
		return 0;
	}

	bool EB_Source::prepare(std::chrono::milliseconds &timeout_ms)
	{
		// Retrieve cached current time
		auto now    = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
		auto epoch  = now.time_since_epoch();
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

		// Work-around for no TX flow control: flush data now
		socket.check(now_ms, 0, &no_fd);

		// remove all filedecriptors from list
		clear_poll(); 
		fds.clear();
		// Find descriptors we need to watch and add them to the list
		socket.descriptors(this, &EB_Source::add_fd); 
		fds_it = fds.begin(); // position the iterator at the beginnin of the vector
		fds_it_valid = true;

		// Determine timeout
		uint32_t timeout = socket.timeout();
		if (timeout) {
			int64_t ms = static_cast<int64_t>(timeout)*1000 - now_ms;
			timeout_ms = (ms < 0) ? std::chrono::milliseconds(0) : std::chrono::milliseconds(ms);
			return (timeout_ms.count() == 0); // true means immediately ready
		} 
		return false;
	}

	bool EB_Source::check()
	{
		bool ready = false;

		// Descriptors ready?
		for (std::vector<pollfd>::iterator i = fds.begin(); i != fds.end(); ++i) {
			if ((i->revents & i->events) != 0) {
				return true;
			}
		}

		// Timeout ready?
		auto now    = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
		auto epoch  = now.time_since_epoch();
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
		ready |= socket.timeout() <= now_ms/1000; // why >= ???

		return ready;
	}

	bool EB_Source::dispatch()
	{
		auto now    = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
		auto epoch  = now.time_since_epoch();
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

		// Process any pending packets
		socket.check(now_ms/1000, this, &EB_Source::get_fd);

		return true;
	}

	std::string EB_Source::type() {
		return "EB_Source";
	}
}