#include "SignalGroup.h"
#include <iostream>

namespace saftlib
{
	SignalGroup globalSignalGroup;

	int wait_for_signal(int timeout_ms) 
	{
		return globalSignalGroup.wait_for_signal(timeout_ms);
	}

	void SignalGroup::add(saftbus::Proxy *proxy, bool automatic_dispatch) 
	{
		_signal_group.push_back(automatic_dispatch?proxy:nullptr);
		struct pollfd pfd;
		pfd.fd = proxy->get_reading_end_of_signal_pipe();
		pfd.events = POLLIN;
		_fds.push_back(pfd);
	}

	void SignalGroup::remove(saftbus::Proxy *proxy) 
	{
		int idx = 0;
		std::vector<saftbus::Proxy*> new_signal_group;
		std::vector<struct pollfd>   new_fds;
		for(auto p: _signal_group) {
			if (p != proxy) {
				new_signal_group.push_back(p);
				new_fds.push_back(_fds[idx]);
			}
			++idx;
		}
		_fds          = new_fds;
		_signal_group = new_signal_group;
	}

	int SignalGroup::wait_for_signal(int timeout_ms)
	{
		int result;
		if ((result = poll(&_fds[0], _fds.size(), timeout_ms)) > 0) {
			int idx = 0;
			for (auto fd: _fds) {
				if (fd.revents & POLLIN) {
					if (_signal_group[idx] != nullptr) {
						_signal_group[idx]->dispatch(Glib::IOCondition());
					}
				}
				++idx;
			}
		}
		return result;
	}

} // namespace saftlib