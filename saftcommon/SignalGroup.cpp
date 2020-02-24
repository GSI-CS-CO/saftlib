#include "SignalGroup.h"
#include <iostream>
#include <time.h>

namespace saftlib
{
	SignalGroup globalSignalGroup;
	SignalGroup noSignals;

	int wait_for_signal(int timeout_ms) 
	{
		return globalSignalGroup.wait_for_signal(timeout_ms);
	}

	void SignalGroup::add(saftbus::Proxy *proxy, bool automatic_dispatch) 
	{
		_signal_group.push_back(automatic_dispatch?proxy:nullptr);
		struct pollfd pfd;
		pfd.fd = proxy->get_reading_end_of_signal_pipe();
		pfd.events = POLLIN | POLLHUP | POLLERR | POLLNVAL;
		_fds.push_back(pfd);
		_sender_alive = true;
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
				if (fd.revents & POLLNVAL || fd.revents & POLLERR || fd.revents & POLLHUP) {
					// saftbus object was removed or saftd was closed or suffered a crash
					throw saftbus::Error(saftbus::Error::ACCESS_DENIED, "saftbus object diappeared");
				} else {
					if (fd.revents & POLLIN) {
						if (_signal_group[idx] != nullptr) {
							_signal_group[idx]->dispatch(Slib::IOCondition());
						}
					}
				}
				++idx;
			}
		} else if (result < 0) {
			// error 
			int errno_ = errno;
			switch(errno_) {
				case EFAULT:
					std::cerr << "saftlib::SignalGroup::wait_for_signal() poll error EFAULT: " << strerror(errno_) << std::endl;
				break;
				case EINTR:
					std::cerr << "saftlib::SignalGroup::wait_for_signal() poll error EINTR: " << strerror(errno_) << std::endl;
				break;
				case EINVAL:
					std::cerr << "saftlib::SignalGroup::wait_for_signal() poll error EINVAL: " << strerror(errno_) << std::endl;
				break;
				case ENOMEM:
					std::cerr << "saftlib::SignalGroup::wait_for_signal() poll error ENOMEM: " << strerror(errno_) << std::endl;
				break;
			}
		} else {
			// timeout was hit. do nothing
		}
		return result;
	}

	bool SignalGroup::sender_alive() 
	{
		return _sender_alive;
	}

} // namespace saftlib