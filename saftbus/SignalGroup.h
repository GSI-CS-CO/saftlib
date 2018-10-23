#ifndef SIGNAL_GROUP_H
#define SIGNAL_GROUP_H

#include <poll.h>

#include <saftbus.h>

namespace saftlib
{


	class SignalGroup
	{
	public:
		void add(Glib::RefPtr<saftbus::Proxy> proxy);
		int wait_for_signal(int timeout_ms = -1);

	private:
		std::vector<Glib::RefPtr<saftbus::Proxy> > _signal_group;
		std::vector<struct pollfd> _fds;
	};

	int wait_for_signal(int timeout_ms = -1);

	extern SignalGroup globalSignalGroup;

} // namespace saftlib

#endif

