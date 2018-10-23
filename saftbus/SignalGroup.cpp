#include "SignalGroup.h"
#include <iostream>

namespace saftlib
{
	SignalGroup globalSignalGroup;

	int wait_for_signal(int timeout_ms) 
	{
		return globalSignalGroup.wait_for_signal(timeout_ms);
	}

	void SignalGroup::add(Glib::RefPtr<saftbus::Proxy> proxy) 
	{
		_signal_group.push_back(proxy);
		struct pollfd pfd;
		pfd.fd = proxy->get_reading_end_of_signal_pipe();
		pfd.events = POLLIN;
		_fds.push_back(pfd);
	}

	int SignalGroup::wait_for_signal(int timeout_ms)
	{
		int result;
		if (result = poll(&_fds[0], _fds.size(), timeout_ms) > 0) {
			int idx = 0;
			for (auto fd: _fds) {
				if (fd.revents & POLLIN) {
					_signal_group[idx]->dispatch(Glib::IOCondition());
				}
				++idx;
			}
		}
		return result;
	}

} // namespace saftlib