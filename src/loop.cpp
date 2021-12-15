#include "loop.hpp"

#include <chrono>
#include <algorithm>
#include <array>
#include <thread>
#include <iostream>
#include <cstring>
#include <cassert>

#include <poll.h>

namespace mini_saftlib {


	struct Source::Impl {
		Loop *loop;
		// store the file descriptor pointers on the stack to be faster
		// drawback: no source may provide more file descriptor than have place in pfds-array
		std::array<struct pollfd*, 32> pfds; 
		size_t                         pfds_size;
	};

	Source::Source() 
		: d(std::make_unique<Impl>())
	{
		d->pfds_size = 0;
	}
	Source::~Source() = default;

	void Source::add_poll(struct pollfd &pfd)
	{
		assert(d->pfds_size < d->pfds.size());
		d->pfds[d->pfds_size++] = &pfd;
	}
	void Source::remove_poll(struct pollfd &pfd)
	{
		d->pfds_size = d->pfds.begin() - std::remove(d->pfds.begin(), d->pfds.end(), &pfd);
	}
	void Source::destroy() {
		d->loop->remove(this);
	}
	bool operator==(const std::unique_ptr<Source> &lhs, const Source *rhs)
	{
		return &*lhs == rhs;
	}

	//////////////////////////////
	//////////////////////////////
	//////////////////////////////

	struct Loop::Impl {
		std::vector<Source*> removed_sources;
		std::vector<std::unique_ptr<Source> > sources;
		std::vector<struct pollfd>  pfds;
		std::vector<struct pollfd*> source_pfds;
		bool run;
	};

	Loop::Loop() 
		: d(std::make_unique<Impl>())
	{
		// reserve all the vectors with enough space to avoid 
		// dynamic allocation in normal operation
		const size_t revserve_that_much = 1024;
		d->removed_sources.reserve(revserve_that_much);
		d->sources.reserve(revserve_that_much);
		d->pfds.reserve(revserve_that_much);
		d->source_pfds.reserve(revserve_that_much);
		d->run = true;
	}
	Loop::~Loop() = default;

	Loop& Loop::get_default() {
		static Loop default_loop;
		return default_loop;
	}

	bool Loop::iteration(bool may_block) {
		std::cerr << ".";
		static const auto no_timeout = std::chrono::milliseconds(-1);

		//////////////////
		// preparation 
		// (find the earliest timeout)
		//////////////////
		d->pfds.clear();
		d->source_pfds.clear();
		auto timeout = no_timeout; 
		for(auto &source: d->sources) {
			auto timeout_source = no_timeout;
			source->prepare(timeout_source);
			if (timeout_source != no_timeout) {
				if (timeout == no_timeout) {
					timeout = timeout_source;
				} else {
					timeout = std::min(timeout, timeout_source);
				}
			}
			for(auto it = source->d->pfds.cbegin(); it != source->d->pfds.cbegin()+source->d->pfds_size; ++it) {
				// create a packed array of pfds that can be passed to poll()
				d->pfds.push_back(**it);
				// also create an array of pointers to pfds to where the poll() results can be copied back
				d->source_pfds.push_back(*it);
			}
		}
		if (!may_block) {
			timeout = std::chrono::milliseconds(0);
		}
		//////////////////
		// polling / waiting
		//////////////////
		// std::cerr << "poll pfds size " << d->pfds.size() << std::endl;
		if (d->pfds.size() > 0) {
			int poll_result = 0;
			if ((poll_result = poll(&d->pfds[0], d->pfds.size(), timeout.count())) > 0) {
				for (unsigned i = 0; i < d->pfds.size();++i) {
					// copy the results back to the owners of the pfds
					// std::cerr << "poll results " << i << " = " << d->pfds[i].revents << std::endl;
					d->source_pfds[i]->revents = d->pfds[i].revents;
				}
			} else if (poll_result < 0) {
				std::cerr << "poll error: " << strerror(errno) << std::endl;
			} 
		} else if (timeout > std::chrono::milliseconds(0)) {
			std::this_thread::sleep_for(timeout);
		}

		//////////////////
		// dispatching
		//////////////////
		for (auto &source: d->sources) {
			if (source->check()) {
				source->dispatch();
			}
		}

		//////////////////
		// cleanup (if needed)
		//////////////////
		for (auto removed_source: d->removed_sources) {
			d->sources.erase(std::remove(d->sources.begin(), d->sources.end(), removed_source), 
				          d->sources.end());			
		}
		d->removed_sources.clear();

		return true;
	}

	void Loop::run() {
		while (d->run) {
			if (!iteration(true)) {
				break;
			}
		}
	}

	bool Loop::quit() {
		return d->run = false;
	}

	bool Loop::quit_in(std::chrono::milliseconds wait_ms) {
		wait_ms = std::max(wait_ms, std::chrono::milliseconds(1)); // no less then 1 ms
		connect(std::move(
				std::make_unique<mini_saftlib::TimeoutSource>
					(sigc::mem_fun(this, &Loop::quit), wait_ms)
			)
		);
		return false;
	}

	bool Loop::connect(std::unique_ptr<Source> source) {
		source->d->loop = this;
		d->sources.push_back(std::move(source));
		return true;
	}

	void Loop::remove(Source *s) {
		d->removed_sources.push_back(s);
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
		if (ms <= std::chrono::milliseconds(0)) {
			return true;
		}
		return false;
	}

	bool TimeoutSource::dispatch() {
		auto now = std::chrono::steady_clock::now();
		do {
			d->next_time += d->interval;
		} while (now >= d->next_time);
		auto result = d->slot();
		if (!result) {
			destroy();
		}
		return result;
	}



	//////////////////////////////
	//////////////////////////////
	//////////////////////////////


	struct IoSource::Impl {
		sigc::slot<bool, int, int> slot;
		struct pollfd pfd;
		int id;
		static int id_source;
	};
	int IoSource::Impl::id_source = 0;

	IoSource::IoSource(sigc::slot<bool, int, int> slot, int fd, int condition) 
		: d(std::make_unique<Impl>())
	{
		d->slot              = slot;
		d->pfd.fd            = fd;
		d->pfd.events        = condition;
		d->pfd.revents       = 0;
		d->id = d->id_source++;
		// std::cerr << "IoSource(" << d->id << ")" << std::endl;
		// std::cerr << (condition & POLLHUP) << " " << (condition & POLLIN) << std::endl;
		add_poll(d->pfd);
	}
	IoSource::~IoSource() 
	{
		// remove_poll(d->pfd);
	}

	bool IoSource::prepare(std::chrono::milliseconds &timeout_ms) {
		// std::cerr << "prepare IoSource(" << d->id << ")" << std::endl;
		if (d->pfd.revents & d->pfd.events) {
			return true;
		}
		return false;
	}

	bool IoSource::check() {
		// std::cerr << "check IoSource(" << d->id << ") " << d->pfd.revents << " "<< d->pfd.events << std::endl;
		if (d->pfd.revents & d->pfd.events) {
			return true;
		}
		return false;
	}

	bool IoSource::dispatch() {
		// std::cerr << "dispatch IoSource(" << d->id << ")" << std::endl;
		auto result = d->slot(d->pfd.fd, d->pfd.revents);
		if (!result) {
			remove_poll(d->pfd);
			destroy();
		}
		d->pfd.revents = 0; // clear the events after  the dispatching
		return result;
	}




}