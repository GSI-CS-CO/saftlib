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
		void wait_for_signal();

	private:
		std::vector<Glib::RefPtr<saftbus::Proxy> > _signal_group;
		std::vector<struct pollfd> _fds;
	};

	void wait_for_signal();

	extern SignalGroup globalSignalGroup;

} // namespace saftlib

#endif

