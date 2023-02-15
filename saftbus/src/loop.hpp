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

#ifndef SAFTBUS_LOOP_HPP_
#define SAFTBUS_LOOP_HPP_

#include <memory>
#include <iostream>
#include <chrono>
#include <functional>
#include <vector>
#include <set>

#include <poll.h>

namespace saftbus {

	class Loop;
	class Source {
		// struct Impl; std::unique_ptr<Impl> d;
	friend class Loop;
	public:
		Source();
		virtual ~Source();
		virtual bool prepare(std::chrono::milliseconds &timeout_ms) = 0;
		virtual bool check() = 0;
		virtual bool dispatch() = 0;
		virtual std::string type() = 0;
		long get_id();
	protected:
		void add_poll(pollfd *pfd);
		void remove_poll(pollfd *pfd);
		void clear_poll();
	private:
		Loop *loop;
		std::vector<pollfd*> pfds;
		static long id_counter;
		long id; 
		bool valid;
	};

	class SourceHandle {
		friend class Loop;
		long source_id;
		long loop_id;
	public:
		SourceHandle() :source_id(0), loop_id(0) {}
		long get_source_id() const {return source_id;}
		long get_loop_id()   const {return loop_id;}
		bool connected()     const {return loop_id!=0;}
	};

	class Loop {
		struct Impl; std::unique_ptr<Impl> d;
	public:
		Loop();
		~Loop();
		bool iteration(bool may_block);
		void run();
		bool quit();
		bool quit_in(std::chrono::milliseconds wait_ms);
		SourceHandle connect(std::unique_ptr<Source> source);

		template<typename T, typename... Args> // T must be subclass of Source
		SourceHandle connect(Args&&... args) {
			return connect(std::move(std::unique_ptr<T>(new T(std::forward<Args>(args)...))));
		}
		void remove(SourceHandle s);
		void clear(); // remove all sources
		static Loop &get_default();
	};

    /////////////////////////////////////
	// Define two useful Source types
    /////////////////////////////////////

    // call <slot> whenever <interval> amount of time has passed.
    // fist execution starts at <inteval>+<offset>
    // source is destroyed if <slot> returns false
	class TimeoutSource : public Source {
		// struct Impl; std::unique_ptr<Impl> d;
	public:
		TimeoutSource(std::function<bool(void)> slot, std::chrono::milliseconds interval, std::chrono::milliseconds offset = std::chrono::milliseconds(0));
		~TimeoutSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
		std::string type() override;
	private:
		std::function<bool(void)> slot;
		std::chrono::milliseconds interval;
		std::chrono::time_point<std::chrono::steady_clock> dispatch_time;		
	};

	// call <slot> whenever <fd> fulfills <condition> (usually POLLIN or POLLOUT)
	// source is destroyed if POLLHP is seen on <fd>
	class IoSource : public Source {
		friend class Loop;
		// struct Impl; std::unique_ptr<Impl> d;
	public:
		IoSource(std::function<bool(int, int)> slot, int fd, int condition);
		~IoSource();
		bool prepare(std::chrono::milliseconds &timeout_ms) override;
		bool check() override;
		bool dispatch() override;
		std::string type() override;
		static std::set<int> all_fds;
	private:
		std::function<bool(int, int)> slot;
		pollfd pfd;
		// int id;
		// static int id_source;		
	};

}


#endif
