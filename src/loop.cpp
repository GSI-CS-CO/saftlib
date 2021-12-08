#include "loop.hpp"

#include <chrono>
#include <algorithm>
#include <thread>
#include <iostream>

#include <poll.h>

namespace mini_saftlib {

	struct Loop::Impl {
		std::vector<std::shared_ptr<Source> > sources;
		std::vector<struct pollfd>  pfds;
		std::vector<struct pollfd*> source_pfds;
	};

	Loop::Loop() 
		: d(std::make_unique<Impl>())
	{
	}
	Loop::~Loop() = default;

	bool Loop::iteration(bool may_block) {
		d->pfds.clear();
		auto timeout = std::chrono::milliseconds(-1); // this value will be passed to the poll system call, -1 means no timeout
		int i = 0;
		for(auto &source: d->sources) {
			auto timeout_source = std::chrono::milliseconds(-1);
			source->prepare(timeout_source);
			if (timeout_source >= std::chrono::milliseconds(0)) {
				if (timeout == std::chrono::milliseconds(-1)) {
					timeout = timeout_source;
				} else {
					timeout = std::min(timeout, timeout_source);
				}
			}
			for(auto it = source->get_pfds().cbegin(); it != source->get_pfds().cend(); ++it) {
				d->pfds.push_back(**it);
				d->source_pfds.push_back(*it);
			}
		}
		if (!may_block) {
			timeout = std::chrono::milliseconds(0);
		}
		if (d->pfds.size() > 0) {
			int poll_result = 0;
			if ((poll_result = poll(&d->pfds[0], d->pfds.size(), timeout.count())) > 0) {
				for (unsigned i = 0; i < d->pfds.size();++i) {
					// copy the results back to the owners of the pfds
					d->source_pfds[i]->revents = d->pfds[i].revents;
				}
			} else {
				// timeout hit
			}
		} else if (timeout > std::chrono::milliseconds(0)) {
			std::this_thread::sleep_for(timeout);
		}
		i = 0;
		for (auto &source: d->sources) {
			if (source->check()) {
				source->dispatch();
			}
			++i;
		}

		return true;
	}

	void Loop::run() {
		for (;;) {
			if (!iteration(true)) {
				break;
			}
		}
	}

	bool Loop::connect_timeout(sigc::slot<bool> slot, std::chrono::milliseconds interval, std::chrono::milliseconds offset)
	{
		d->sources.push_back(std::make_shared<TimeoutSource>(slot, interval, offset));
		return true;
	}

	bool Loop::connect_io(sigc::slot<bool, int, int> slot, int fd, int condition) {
		d->sources.push_back(std::make_shared<IoSource>(slot,fd,condition));
		return true;
	}



	//////////////////////////////
	//////////////////////////////
	//////////////////////////////

	struct Source::Impl {
		std::vector<struct pollfd*> pfds;
	};

	const std::vector<struct pollfd*> &Source::get_pfds() const {
		return d->pfds;
	}

	Source::Source() 
		: d(std::make_unique<Impl>())
	{}
	Source::~Source() = default;

	void Source::add_poll(struct pollfd &pfd)
	{
		d->pfds.push_back(&pfd);
	}
	void Source::remove_poll(struct pollfd &pfd)
	{
		d->pfds.erase(std::remove(d->pfds.begin(), d->pfds.end(), &pfd), 
			          d->pfds.end());
	}

	//////////////////////////////
	//////////////////////////////
	//////////////////////////////


	struct TimeoutSource::Impl {
		sigc::slot<bool> slot;
		std::chrono::milliseconds interval;
		std::chrono::time_point<std::chrono::steady_clock> next_time;
		Impl(sigc::slot<bool> s, std::chrono::milliseconds i, std::chrono::milliseconds o) : slot(s), interval(i), next_time(std::chrono::steady_clock::now()+i+o)
		{}
	};

	TimeoutSource::TimeoutSource(sigc::slot<bool> slot, std::chrono::milliseconds interval, std::chrono::milliseconds offset) 
		: d(std::make_unique<Impl>(slot, interval, offset))
	{}

	TimeoutSource::~TimeoutSource() = default;

	bool TimeoutSource::prepare(std::chrono::milliseconds &timeout_ms) {
		auto now = std::chrono::steady_clock::now();
		if (now >= d->next_time) {
			timeout_ms = std::chrono::milliseconds(0);
			return true;
		}
		timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(d->next_time - now); 
		return false;
	}

	bool TimeoutSource::check() {
		auto now = std::chrono::steady_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d->next_time - now); 
		if (ms == std::chrono::milliseconds(0)) {
			return true;
		}
		return false;
	}

	bool TimeoutSource::dispatch() {
		auto now = std::chrono::steady_clock::now();
		do {
			d->next_time += d->interval;
		} while (now >= d->next_time);
		return d->slot();
	}



	//////////////////////////////
	//////////////////////////////
	//////////////////////////////


	struct IoSource::Impl {
		sigc::slot<bool, int, int> slot;
		struct pollfd pfd;
		Impl(sigc::slot<bool, int, int> s, int fd, int condition) 
			: slot(s)
		{
			pfd.fd = fd;
			pfd.events = condition;
			pfd.revents = 0;
		}
	};

	IoSource::IoSource(sigc::slot<bool, int, int> slot, int fd, int condition) 
		: d(std::make_unique<Impl>(slot, fd, condition))
	{
		add_poll(d->pfd);
	}
	IoSource::~IoSource() = default;

	bool IoSource::prepare(std::chrono::milliseconds &timeout_ms) {
		if (d->pfd.revents & POLLIN) {
			return true;
		}
		if (d->pfd.revents & POLLHUP) {
			remove_poll(d->pfd);
		}
		return false;
	}

	bool IoSource::check() {
		if (d->pfd.revents & POLLIN) {
			return true;
		}
		return false;
	}

	bool IoSource::dispatch() {
		return d->slot(d->pfd.fd, d->pfd.revents);
	}




}