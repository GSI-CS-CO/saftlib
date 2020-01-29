#ifndef SLIB_POLLFD_H
#define SLIB_POLLFD_H

#include <sigc++/sigc++.h>
#include <cstdint>
#include <poll.h>

namespace Slib
{

	typedef int IOCondition;
	static const int IO_IN   = POLLIN;   // There is data to read.
	static const int IO_OUT  = POLLOUT;  // Data can be written (without blocking).
	static const int IO_PRI  = POLLPRI;  // There is urgent data to read.
	static const int IO_ERR  = POLLERR;  // Error condition.
	static const int IO_HUP  = POLLHUP;  // Hung up (the connection has been broken, usually for pipes and sockets).
	static const int IO_NVAL = POLLNVAL; // Invalid request. The file descriptor is not open. 

	class PollFD
	{
	public:
		PollFD ();
		PollFD (int fd);
		PollFD (int fd, IOCondition events);
		void 		set_fd(int fd);
		int 		get_fd() const;
		void 		set_events(IOCondition events);
		IOCondition get_events() const;
		void 		set_revents(IOCondition revents);
		IOCondition get_revents() const;

		bool operator==(const PollFD &rhs);

		struct pollfd pfd;
	};

} // namespace Slib

#endif

