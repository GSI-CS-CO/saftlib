#include "loop.hpp"

#include <chrono>
#include <algorithm>
#include <thread>

#include <poll.h>

namespace mini_saftlib {

	struct Loop::Impl {
		std::vector<std::shared_ptr<Source> > sources;
	};

	Loop::Loop() 
		: d(std::make_unique<Impl>())
	{
	}
	Loop::~Loop() = default;

	bool Loop::iteration(bool may_block) {
		auto now = std::chrono::steady_clock::now();
		
		auto timeout = std::chrono::milliseconds(-1); // this value will be passed to the poll system call, -1 means no timeout
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
		}

		if (!may_block) {
			timeout = std::chrono::milliseconds(0);
		}

		// do the poll system call
		if (timeout > std::chrono::milliseconds(0)) {
			std::this_thread::sleep_for(timeout);
		}

		for (auto &source: d->sources) {
			if (source->check()) {
				source->dispatch();
			}
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
		if (now >= d->next_time) {
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

}