/** Copyright (C) 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */

#include "loop.hpp"

#include <chrono>
#include <algorithm>
#include <array>
#include <thread>
#include <iostream>
#include <cstring>
#include <cassert>
#include <sstream>

#include <poll.h>

namespace saftbus {


	Source::Source() 
	{
		//===std::cerr << "+++++++++++++++++    Sourc::Source()" << std::endl;
		if (id_counter == -1) ++id_counter; // prevent id_counter to produce an id of 0 (no source should have id 0)
		id = ++id_counter;
		id |= ((long)rand()%0xffffffff)<<32;
		valid = true;
	}
	Source::~Source() {
		//===std::cerr << "Source::~Source()" << std::endl;
	}

	void Source::add_poll(pollfd *pfd)
	{
		// //===std::cerr << "add poll " << pfd->fd << std::endl;
		pfds.push_back(pfd);
	}
	void Source::remove_poll(pollfd *pfd)
	{
		// //===std::cerr << "remove poll " << pfd->fd << std::endl;
		pfds.erase(pfds.begin(), std::remove(pfds.begin(), pfds.end(), pfd));
	}
	void Source::clear_poll() {
		pfds.clear();
	}
	long Source::get_id() {
		return id;
	}

	long Source::id_counter = 0;

	//////////////////////////////
	//////////////////////////////
	//////////////////////////////

	struct Loop::Impl {
		std::vector<std::unique_ptr<Source> > added_sources;
		std::vector<std::unique_ptr<Source> > sources;
		bool running;
		int running_depth; 
		long id;
		static long id_counter;
	};
	long Loop::Impl::id_counter = 0;

	
	Loop::Loop() 
		: d(new Impl)
	{
		// reserve all the vectors with enough space to avoid 
		// dynamic allocation in normal operation
		const size_t revserve_that_much = 32;
		d->added_sources.reserve(revserve_that_much);
		d->sources.reserve(revserve_that_much);
		d->running = true;
		d->running_depth = 0; // 0 means: the loop is not running
		if (d->id_counter == -1) ++d->id_counter; // prevent d->id_counter to produce an id of 0 (no source should have id 0)
		d->id = ++d->id_counter;
		d->id |= ((long)rand()%0xffffffff)<<32;
	}
	Loop::~Loop() {
		// //===std::cerr << "~Loop()" << std::endl;
		d->sources.clear();
		d->added_sources.clear();
	}

	Loop& Loop::get_default() {
		static Loop default_loop;
		return default_loop;
	}

	bool Loop::iteration(bool may_block) {
		++d->running_depth;
		// //===std::cerr << "Loop::iteration " << d->running_depth << std::endl;
		static const auto no_timeout = std::chrono::milliseconds(-1);
		std::vector<struct pollfd> pfds;
		// pfds.reserve(16);
		std::vector<struct pollfd*> source_pfds;
		// source_pfds.reserve(16);
		auto timeout = no_timeout; 

		// unsigned us = 0;
		auto start = std::chrono::steady_clock::now();
		auto stop = std::chrono::steady_clock::now();

		// //===std::cerr << "sources.size() = " << d->sources.size() << std::endl;

		//////////////////
		// preparation 
		// (find the earliest timeout)
		//////////////////
		for(auto &source: d->sources) {
			if (!source->valid) continue; 

			auto timeout_from_source = no_timeout;
			source->prepare(timeout_from_source); // source may leave timeout_from_source unchanged 
			if (timeout_from_source != no_timeout) {
				if (timeout == no_timeout) {
					timeout = timeout_from_source;
				} else {
					timeout = std::min(timeout, timeout_from_source);
				}
			}
			for(auto it = source->pfds.cbegin(); it != source->pfds.cbegin()+source->pfds.size(); ++it) {
				// create a packed array of pfds that can be passed to poll()
				pfds.push_back(**it);
				// also create an array of pointers to pfds to where the poll() results can be copied back
				source_pfds.push_back(*it);
				// //===std::cerr << "added fd=" << (*it)->fd << std::endl;
			}
		}
		if (!may_block) { 
			timeout = std::chrono::milliseconds(0);
		}
		//////////////////
		// polling / waiting
		//////////////////
		// //===std::cerr << "poll pfds size " << pfds.size() << std::endl;
		if (pfds.size() > 0) {
			// //===std::cerr << "polling timeout_ms = " << timeout.count() << std::endl;
			int poll_result = 0;
			// stop = std::chrono::steady_clock::now();
			// us += std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count();
			if ((poll_result = poll(&pfds[0], pfds.size(), timeout.count())) > 0) {
				// //===std::cerr << "poll result = " << poll_result << std::endl;
				// copy the results back to the owners of the pfds
				for (unsigned i = 0; i < pfds.size();++i) {
					source_pfds[i]->revents = pfds[i].revents;
					// if (pfds[i].revents & POLLHUP) {
					// 	//===std::cerr << "POLLHUP on pfds[" << i << "]" << std::endl;
					// }
					// if (pfds[i].revents & POLLERR) {
					// 	//===std::cerr << "POLLERR on pfds[" << i << "]" << std::endl;
					// }
				}
			} else if (poll_result < 0) {
				//===std::cerr << "poll error: " << strerror(errno) << std::endl;
			} else {
				// //===std::cerr << "poll result = " << poll_result << std::endl;
			}
			start = std::chrono::steady_clock::now();

		} else if (timeout > std::chrono::milliseconds(0)) {
			// stop = std::chrono::steady_clock::now();
			// us += std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count();

			// //===std::cerr << "sleeping" << std::endl;
			std::this_thread::sleep_for(timeout);
			start = std::chrono::steady_clock::now();
			
		}
		// start = std::chrono::steady_clock::now();

		//////////////////
		// dispatching
		//////////////////
		for (auto &source: d->sources) {
			if (!source->valid) continue;

			if (source->check()) { // if check returns true, dispatch is called
				// //===std::cerr << "Loop::iteration dispatching to " << source->type() << std::endl;
				if (!source->dispatch()) { // if dispatch returns false, the source is removed
					source->valid = false;
				}
			}
		}

		//////////////////////////////////////////////////////
		// cleanup of finished sources
		// and addition of new sources
		// only if this is not a nested iteration
		//////////////////////////////////////////////////////
		// bool changes = false;
		if (d->running_depth == 1) {
			// //===std::cerr << "cleaning up sources" << std::endl;
			d->sources.erase(std::remove_if(d->sources.begin(), d->sources.end(), [](std::unique_ptr<Source> &s){return !s->valid;}), 
				          d->sources.end());

			// adding new sources
			for (auto &added_source: d->added_sources) {
				// //===std::cerr << "adding a source" << std::endl;
				if (added_source->valid) {
					d->sources.push_back(std::move(added_source));
					// changes = true;
				}
			}
			d->added_sources.clear();
		}

		--d->running_depth;

		stop = std::chrono::steady_clock::now();
		// //===std::cerr << changes << "    " << us << " , " << std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count() << std::endl;



		return !d->sources.empty();
	}

	void Loop::run() {
		d->running = true;
		while (d->running) {
			if (!iteration(true)) {
				d->running = false;
			}
		}
	}

	bool Loop::quit() {
		return d->running = false;
	}

	bool Loop::quit_in(std::chrono::milliseconds wait_ms) {
		// //wait_ms = std::max(wait_ms, std::chrono::milliseconds(1)); // no less then 1 ms
		// connect(std::move(
		// 		std2::make_unique<saftbus::TimeoutSource>
		// 			(std::bind(&Loop::quit, this), wait_ms)
		// 	)
		// );
		connect<saftbus::TimeoutSource>(std::bind(&Loop::quit, this), wait_ms, wait_ms);
		return false;
	}


	SourceHandle Loop::connect(std::unique_ptr<Source> source) {
		// //===std::cerr << "Loop::connect" << std::endl;
		source->loop = this;
		SourceHandle result;
		result.loop_id   = d->id;
		result.source_id = source->id;

		if (d->running_depth) {
			// durin an iteration, the source vector may not be changed.
			// put the source in a buffer vector which is cpoied into 
			// the source vector after the iteration is done
			d->added_sources.push_back(std::move(source));
		} else {
			d->sources.push_back(std::move(source));
		}
		return result;
	}

	bool operator==(const std::unique_ptr<Source> &lhs, const SourceHandle &rhs) {
		return lhs->get_id() == rhs.get_source_id();
	}
	/// @brief public version of remove which works with a SourceHandle
	/// @param s the source handle returned from the connect method
	void Loop::remove(SourceHandle s) {
		if (s.loop_id == d->id) { // make sure s was connected to this loop
			auto source = d->sources.begin();
			if ((source=std::find(source, d->sources.end(), s)) != d->sources.end()) {
				//===std::cerr << "Loop::remove found in sources" << std::endl;
				(*source)->valid = false;
			}
			source = d->added_sources.begin();
			if ((source=std::find(source, d->added_sources.end(), s)) != d->added_sources.end()) {
				//===std::cerr << "Loop::remove found in added sources" << std::endl;
				(*source)->valid = false;
			}
		}
	}

	void Loop::clear() {
		//===std::cerr << "************ Loop::clear()" << std::endl;
		d->sources.clear();
		d->added_sources.clear();
	}


	//////////////////////////////
	//////////////////////////////
	//////////////////////////////


	TimeoutSource::TimeoutSource(std::function<bool(void)> s, std::chrono::milliseconds i, std::chrono::milliseconds o) 
		: slot(s), interval(i), dispatch_time(std::chrono::steady_clock::now()+o)
	{
		if (interval <= std::chrono::milliseconds(0)) {
			interval = std::chrono::milliseconds(1);
		}
	}

	TimeoutSource::~TimeoutSource() {
		//===std::cerr << "TimeoutSource::~TimeoutSource()" << std::endl;
	}

	// prepare is called by Loop to find out the next dispatch_time of all sources
	// and it will wait for min() of all reported dispatch_times.
	bool TimeoutSource::prepare(std::chrono::milliseconds &timeout_ms) {
		auto now = std::chrono::steady_clock::now();
		timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dispatch_time - now); 
		if (timeout_ms.count() <= 0) {
			timeout_ms = std::chrono::milliseconds(0);
			return true;
		}
		return false;
	}

	// After the wait phase, the loop will call check on all source.
	// Loop will call dispatch on all sources where check returns true. 
	bool TimeoutSource::check() {
		auto now = std::chrono::steady_clock::now();
		auto timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dispatch_time - now); 
		if (timeout_ms.count() <= 0) {
			return true;
		}
		return false;
	}

	// Execute whatever action is attached to the source
	bool TimeoutSource::dispatch() {
		auto now = std::chrono::steady_clock::now();
		do {
			dispatch_time += interval;
		} while (now >= dispatch_time);
		return slot();
	}

	std::string TimeoutSource::type() {
		std::ostringstream result;
		result << "TimeoutSource ";
		return result.str();
	}

	//////////////////////////////
	//////////////////////////////
	//////////////////////////////


	IoSource::IoSource(std::function<bool(int, int)> s, int f, int c) 
		: slot(s)
	{
		pfd.fd            = f;
		pfd.events        = c;
		pfd.revents       = 0;
		add_poll(&pfd);
		if (all_fds.find(f) != all_fds.end()) assert(false);
		all_fds.insert(f);
	}
	IoSource::~IoSource() 
	{
		remove_poll(&pfd);
		if (all_fds.find(pfd.fd) == all_fds.end()) assert(false);
		all_fds.erase(pfd.fd);
	}

	bool IoSource::prepare(std::chrono::milliseconds &timeout_ms) {
		if (pfd.revents & pfd.events) {
			return true;
		}
		return false;
	}

	bool IoSource::check() {
		// std::cout << "IoSource::check() " << pfd.fd << " "; 
		// if (pfd.revents & POLLIN)  { std::cout << "POLLIN " ; }
		// if (pfd.revents & POLLHUP) { std::cout << "POLLHUP "; }
		// if (pfd.revents & POLLERR) { std::cout << "POLLERR "; }
		// std::cout << std::endl;
		if (pfd.revents & pfd.events) {
			return true;
		}
		return false;
	}

	bool IoSource::dispatch() {
		// std::cout << "IoSource::dispatch() " << pfd.fd << " " << std::endl;
		auto result = slot(pfd.fd, pfd.revents);
		pfd.revents = 0; // clear the events after  the dispatching
		return result;
	}

	std::string IoSource::type() {
		std::ostringstream result;
		result << "IoSource " << pfd.fd;
		return result.str();
	}


	std::set<int> IoSource::all_fds;

}