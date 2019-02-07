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
		// bool 	pending (); // 	Checks if any sources have pending events for the given context.
		// void 	wakeup (); //  	If context is currently waiting in a poll(), interrupt the poll(), and continue the iteration process.
		// bool 	acquire (); // 	Tries to become the owner of the specified context.
		// bool 	wait (Glib::Cond& cond, Glib::Mutex& mutex); // 	Tries to become the owner of the specified context, as with acquire().
		// void 	release (); // 	Releases ownership of a context previously acquired by this thread with acquire().
		// bool 	prepare (int& priority); // 	Prepares to poll sources within a main loop.
		// bool 	prepare (); // 	Prepares to poll sources within a main loop.
		// void 	query (int max_priority, int& timeout, std::vector< PollFD >& fds); // 	Determines information necessary to poll this main loop.
		// bool 	check (int max_priority, std::vector< PollFD >& fds); // 	Passes the results of polling back to the main loop.
		// void 	dispatch (); // 	Dispatches all pending sources.
		// void 	set_poll_func (GPollFunc poll_func); // 	Sets the function to use to handle polling of file descriptors.
		// GPollFunc 	get_poll_func (); // 	Gets the poll function set by g_main_context_set_poll_func().
		//void 	add_poll (PollFD& fd, int priority); // 	Adds a file descriptor to the set of file descriptors polled for this context.
		//void 	remove_poll (PollFD& fd); // 	Removes file descriptor from the set of file descriptors to be polled for a particular context.
		// SignalTimeout 	signal_timeout (); // 	Timeout signal, attached to this MainContext.
		// SignalIdle 	signal_idle (); // 	Idle signal, attached to this MainContext.
		//SignalIO 	signal_io (); // 	I/O signal, attached to this MainContext.
		// SignalChildWatch 	signal_child_watch (); // 	child watch signal, attached to this MainContext.
		void 	reference () const;
		void 	unreference () const;
		// GMainContext* 	gobj ();
		// const GMainContext* 	gobj () const;
		// GMainContext* 	gobj_copy () const;
		//Static Public Member Functions
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

		std::map<unsigned, std::shared_ptr<Source> > sources; // map source_id to source
		friend class Source;

		static std::shared_ptr< MainContext > default_context;
		static bool default_created;
		bool during_iteration;

		std::vector<struct pollfd>                  signal_io_pfds;
		std::vector<sigc::slot<bool, IOCondition> > signal_io_slots;
		std::vector<struct pollfd>                  added_signal_io_pfds;
		std::vector<sigc::slot<bool, IOCondition> > added_signal_io_slots;

		std::vector<int>                            signal_timeout_intervals;
		std::vector<int>                            signal_timeout_time_left;
		std::vector<std::shared_ptr<sigc::slot<bool> > >  signal_timeout_slots;
		std::vector<sigc::connection >              signal_timeout_connections;
		std::vector<int>                            added_signal_timeout_intervals;
		std::vector<std::shared_ptr<sigc::slot<bool> > >  added_signal_timeout_slots;
		std::vector<sigc::connection >              added_signal_timeout_connections;
	};

	MainContext& signal_io();
	MainContext& signal_timeout();



} // namespace Slib

#endif
