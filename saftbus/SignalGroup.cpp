#include "SignalGroup.h"

namespace saftlib
{

	void SignalGroup::add(Glib::RefPtr<saftbus::Proxy> proxy) 
	{
		_signal_group.push_back(proxy);
		struct pollfd pfd;
		pfd.fd = proxy->get_reading_end_of_signal_pipe();
		pfd.events = POLLIN;
		_fds.push_back(pfd);
	}

	void SignalGroup::wait_for_signal()
	{
		if (poll(&_fds[0], _fds.size(), -1) > 0) {
			int idx = 0;
			for (auto fd: _fds) {
				if (fd.revents & POLLIN) {
					_signal_group[idx]->dispatch(Glib::IOCondition());
				}
				++idx;
			}
		}		
	}

} // namespace saftlib