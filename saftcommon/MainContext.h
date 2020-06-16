/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#ifndef SLIB_MAIN_CONTEXT_H
#define SLIB_MAIN_CONTEXT_H

#include <sigc++/sigc++.h>
#include <cstdint>
#include "PollFD.h"


#include <memory>

#include <vector>
#include <map>


namespace Slib
{

	enum Priority
	{
		PRIORITY_LOW,
		PRIORITY_DEFAULT,
		PRIORITY_HIGH,
	};

	class Source;

	class MainContext
	{
	public:
		bool 	iteration (bool may_block); // 	Runs a single iteration for the given main loop.
		void 	reference () const;
		void 	unreference () const;

		static std::shared_ptr< MainContext > 	create (); // 	Creates a new MainContext.
		static std::shared_ptr< MainContext > 	get_default (); // 	Returns the default main context. 

		MainContext& self();

		// implemented here instead for simplicity. In Glibmm this function is part of Glib::SignalIO class.
		// can be used to attach file descriptors 
		sigc::connection connect(sigc::slot<bool, IOCondition> slot, int fd, IOCondition condition, Priority priority=PRIORITY_DEFAULT);

		// connect timeouts 
		sigc::connection connect(sigc::slot<bool> slot, unsigned interval, Priority priority=PRIORITY_DEFAULT);

		unsigned id_counter;
	private:

		void iteration_recursive();

		std::map<unsigned, std::shared_ptr<Source> > sources; // map source_id to source
		friend class Source;

		static std::shared_ptr< MainContext > default_context;
		static bool default_created;
		bool during_iteration;

		std::vector<struct pollfd>                  signal_io_pfds;
		std::vector<sigc::slot<bool, IOCondition> > signal_io_slots;
		std::vector<struct pollfd>                  added_signal_io_pfds;
		std::vector<sigc::slot<bool, IOCondition> > added_signal_io_slots;

		struct Timeout {
			int interval;
			int time_left;
			std::shared_ptr<sigc::slot<bool> > slot;
			sigc::connection connection;
			bool remove;
			Timeout(int iv, std::shared_ptr<sigc::slot<bool> > sl) 
				: interval(iv), time_left(iv), slot(sl), connection(*sl), remove(false) {}
		};
		// std::vector<int>                            signal_timeout_intervals;
		// std::vector<int>                            signal_timeout_time_left;
		// std::vector<std::shared_ptr<sigc::slot<bool> > >  signal_timeout_slots;
		// std::vector<sigc::connection >              signal_timeout_connections;
		std::vector<Timeout> signal_timeouts;
		std::vector<Timeout> added_signal_timeouts;

		// std::vector<int>                            added_signal_timeout_intervals;
		// std::vector<std::shared_ptr<sigc::slot<bool> > >  added_signal_timeout_slots;
		// std::vector<sigc::connection >              added_signal_timeout_connections;
	};

	MainContext& signal_io();
	MainContext& signal_timeout();



} // namespace Slib

#endif
