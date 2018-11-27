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

	// block until a signal on a "non-Glib::MainLoop connected"
	// saftbus::Proxy arrives, as created with the 
	// saftbus::PROXY_FLAGS_ACTIVE_WAIT_FOR_SIGNAL ProxyFlag
	// returns 0 if the timeout was hit
	//         nonzero otherwise
	// timeout of -1 mean: do not timeout
	// timeout is given in units of milliseconds
	int wait_for_signal(int timeout_ms = -1);

	extern SignalGroup globalSignalGroup;

} // namespace saftlib

#endif

