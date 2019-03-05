#ifndef SIGNAL_GROUP_H
#define SIGNAL_GROUP_H

#include <poll.h>

#include <saftbus.h>

namespace saftlib
{


	class SignalGroup
	{
	public:
		void add(saftbus::Proxy *proxy, bool automatic_dispatch = true);
		void remove(saftbus::Proxy *proxy);
		int wait_for_signal(int timeout_ms = -1);

	private:
		std::vector<saftbus::Proxy* > _signal_group;
		std::vector<struct pollfd> _fds;
	};

	// Block until a signal of a connected saftbus::Proxy arrives.
	// Connections between SignalGroup objects and proxies are established by
	// passing the SignalGroup object to the Proxy_create() function.
	// If no SignalGroup is connected explicitly, the globalSignalGroup is used.
	//
	// returns 0 if the timeout was hit
	//         nonzero otherwise
	// timeout is given in units of milliseconds
	// timeout of -1 means: no timeout
	int wait_for_signal(int timeout_ms = -1);

	extern SignalGroup globalSignalGroup;

} // namespace saftlib

#endif

