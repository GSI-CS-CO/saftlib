
#include "MainContext.h"
#include "Source.h"

#include <algorithm>
#include <iostream>

#include <time.h>
#include <unistd.h>

namespace Slib
{

		std::shared_ptr< MainContext > MainContext::default_context;
		bool MainContext::default_created = false;

		MainContext& MainContext::self() {
			return *this;
		}

		void MainContext::iteration_recursive() 
		{
			std::vector<struct pollfd> source_pfds;
			std::vector<PollFD*>  source_pfds_ptr;
			for(auto &id_source: sources) {
				//auto id     = id_source.first;
				auto source = id_source.second;
				int  source_timeout_ms = -1;

				if (source->prepare(source_timeout_ms)) {
					// nested calls to MainContext::iteration() may happen here!
					// but here nothing evil will happen 
					source->dispatch(&source->_slot);
				}
				for (auto pfd: source->_pfds) {
					source_pfds.push_back(pfd->pfd);
					source_pfds_ptr.push_back(pfd);
				}
			}
			poll(&source_pfds[0], source_pfds.size(), 0); // poll with timeout of 0
			for (unsigned idx = 0; idx < source_pfds.size(); ++idx) {
				source_pfds_ptr[idx]->pfd.revents = source_pfds[idx].revents;
			}
			for (auto &id_source: sources) {
				//auto id     = id_source.first;
				auto source = id_source.second;
				if (source->check()) {
					source->dispatch(&source->_slot);
				}
			}
		}


		 // 	Runs a single iteration for the given main loop.
		bool MainContext::iteration (bool may_block)
		{
			// std::cerr << "." << std::endl;
			// was hier passieren muss:
			// - alle fds einsammeln und in ein Array stopfen
			// - poll() aufrufen
			// - Ergebnisse an sources zurueckliefern

			// This if statement treats the special case when any of the dispatch causes an indirect recursion
			// of MainContext::iteration().
			// This is typically done to implement a blocking wait until some hardware condition is triggered.
			// To allow this use case, recursive calls have a special treatment, only testing Source objects,
			// not any of signal_io or signal_timeout.
			// No timeouts will ever be triggered while doing a recursive iteration() of a MainContext
			if (during_iteration == true) {
				iteration_recursive();
				return false;
			}

			struct timespec start, stop;
			clock_gettime(CLOCK_MONOTONIC, &start);
			during_iteration = true;
			//std::cerr << "MainContext::iteration()" << std::endl;

			// poll all fds from all sources
			bool result = false;
			std::vector<struct pollfd> all_pfds(signal_io_pfds.begin(), signal_io_pfds.end()); // copy all signal_io_pfds
			std::vector<PollFD*>  all_pfds_ptr;
			std::vector<unsigned> all_ids;
			int timeout_ms = -1;
			// prepare sources and calculate timeout
			for(auto &id_source: sources) {
				//auto id     = id_source.first;
				auto source = id_source.second;
				int  source_timeout_ms = -1;

				if (source->prepare(source_timeout_ms)) {
					// nested calls to MainContext::iteration() may happen here!
					// but here nothing evil will happen
					source->dispatch(&source->_slot);
				}

				if (source_timeout_ms >= 0) {
					if (timeout_ms >= 0) {
						timeout_ms = std::min(timeout_ms, source_timeout_ms); // minimum timeout_ms
					} else {
						timeout_ms = source_timeout_ms; // first assigment to timeout that is not -1;
					}
				}
				for(auto pfd: source->_pfds) {
					all_pfds.push_back(pfd->pfd);
					all_pfds_ptr.push_back(pfd);
					all_ids.push_back(source->get_id());
				}
			}
			// calculate timeout from signal_timeout_intervals
			for (unsigned i = 0; i < signal_timeouts.size(); ++i ) {
				if (timeout_ms == -1 || signal_timeouts[i].time_left < timeout_ms) {
					timeout_ms = signal_timeouts[i].time_left;
				}
			}
			if (may_block == false) {
				timeout_ms = 0;
			}

			std::vector<unsigned> signal_io_removed_indices;
			int poll_result;
			if ((poll_result = poll(&all_pfds[0], all_pfds.size(), timeout_ms)) > 0) {
				// during check of results, signal_io_pfds and fds from sources
				//  are distinguished using the index.
				unsigned idx = 0;
				for (auto fd: all_pfds) {
					if (idx < signal_io_pfds.size()) {
						// signal_io_pfds
						// copy back
						signal_io_pfds[idx] = fd;
						if (fd.revents & POLLNVAL || fd.revents & POLLERR) {
							// throw out invalid fds from the poll list
							signal_io_removed_indices.push_back(idx);
						}
						if (fd.events & fd.revents) {
							bool result = false;
							if (fd.events & POLLIN) {
								//execute  signal_io callback
								// nested calls to MainContext::iteration() may happen here!
								result = signal_io_slots[idx](fd.revents);
							} 
							if (result == false ) { // add this index to the removal list
								signal_io_removed_indices.push_back(idx);
							}
						}
					} else {
						// source pfds
						// copy the results back into the PollFD 
						all_pfds_ptr[idx-signal_io_pfds.size()]->pfd.revents = all_pfds[idx].revents;
					}
					++idx;
				}
				// update the timeouts
				clock_gettime(CLOCK_MONOTONIC, &stop);
				int dt_ms = (stop.tv_sec - start.tv_sec)*1000 
				               + (stop.tv_nsec - start.tv_nsec)/1000000;

				for (unsigned i = 0; i < signal_timeouts.size(); ++i) {
					if (signal_timeouts[i].time_left >= dt_ms) {
						signal_timeouts[i].time_left -= dt_ms;
					} else {
						signal_timeouts[i].time_left = 0;
					}
					// std::cerr << "timeout " << i << "  :  " << signal_timeout_time_left[i] << std::endl;
				}

			}
			bool any_timeout_is_0 = false;
			for (unsigned i = 0; i < signal_timeouts.size(); ++i) {
				if (signal_timeouts[i].time_left == 0) {
					any_timeout_is_0 = true;
					break;
				}
			}
			if (poll_result == 0 || any_timeout_is_0) { // poll timed out
				//std::cerr << "poll timed out" << std::endl;
				bool need_cleanup = false;
				for (unsigned i = 0; i < signal_timeouts.size(); ++i) {
					// subtract the timeout_ms as used in the poll call
					if (poll_result == 0) { // if poll resulted in timeout, the timeout value must be subtracted
						if (signal_timeouts[i].time_left >= timeout_ms) {
							signal_timeouts[i].time_left -= timeout_ms;
						} else {
							signal_timeouts[i].time_left = 0;
						}
					}
					// std::cerr << signal_timeouts[i].time_left << " ";

					// std::cerr << "check timeout[" << i << "/" << signal_timeout_time_left.size() 
					//           << "] time_left = " << signal_timeout_time_left[i] 
					//           << "  slot.empty() = " << signal_timeout_connections[i].empty()
					//           << std::endl;
					if (signal_timeouts[i].time_left == 0) {
						// execute the callback
						bool callback_result = false;

						if (!signal_timeouts[i].connection.empty()) {
							try {
								callback_result = (*signal_timeouts[i].slot)(); 
							} catch (...) {
								// if this threw any exception, it the source will be removed
								callback_result = false; 
							}
						}
						if (callback_result) {
							// std::cerr << "refresh timeout" << std::endl;
							signal_timeouts[i].time_left = signal_timeouts[i].interval;
						} else {
							need_cleanup = true;
						}
					}
					// std::cerr << "timeout " << i << "  :  " << signal_timeout_time_left[i] << std::endl;
				}
				if (need_cleanup) {
					//std::cerr << "cleaning up: size = " << signal_timeouts.size() << std::endl;
					signal_timeouts.erase(remove_if(signal_timeouts.begin(), signal_timeouts.end(),
						                            [](const Timeout &to){return to.time_left == 0;} ), signal_timeouts.end());
					//std::cerr << "cleaning up done : size = " << signal_timeouts.size() << std::endl;
				}
				//std::cerr << "\n";
			}

			for (auto &id_source: sources) {
				//auto id     = id_source.first;
				auto source = id_source.second;
				if (source->check()) {
					source->dispatch(&source->_slot);
				}
			}

			// remove all signal_ios whose callbacks returned false
			if (signal_io_removed_indices.size() > 0) {
				std::vector<struct pollfd>                  new_signal_io_pfds;
				std::vector<sigc::slot<bool, IOCondition> > new_signal_io_slots;
				for (unsigned i = 0; i < signal_io_pfds.size(); ++i) {
					bool found_in_removal_list = false;
					for (unsigned n = 0; n < signal_io_removed_indices.size(); ++n) {
						if (i == signal_io_removed_indices[n]) {
							found_in_removal_list = true;
							break;
						}
					}
					if (!found_in_removal_list) {
						new_signal_io_pfds.push_back(signal_io_pfds[i]);
						new_signal_io_slots.push_back(signal_io_slots[i]);
					}
				}
				signal_io_slots = new_signal_io_slots;
				signal_io_pfds  = new_signal_io_pfds;
			}

			if (added_signal_timeouts.size() > 0) {
				signal_timeouts.insert(signal_timeouts.end(), added_signal_timeouts.begin(), added_signal_timeouts.end());
				added_signal_timeouts.clear();
			}

			// add the newly created signal_ios
			if (added_signal_io_pfds.size() > 0) {
				for (unsigned i = 0; i < added_signal_io_pfds.size(); ++i) {
					signal_io_pfds.push_back(added_signal_io_pfds[i]);
					signal_io_slots.push_back(added_signal_io_slots[i]);
				}
				added_signal_io_pfds.clear();
				added_signal_io_slots.clear();				
			}

			during_iteration = false;

			return result; // don't know the meaning of that yet
		}


		void 	MainContext::reference () const
		{
			// nothing
		}

		void 	MainContext::unreference () const
		{
			// nothing
		}



		//Static Public Member Functions
		 // 	Creates a new MainContext.
		std::shared_ptr< MainContext > 	MainContext::create ()
		{
			// std::shared_ptr<MainContext> new_context(new MainContext);
			//new_context->id_counter = 0;
			// return new_context;
			return get_default();
		}

		 // 	Returns the default main context. 
		std::shared_ptr< MainContext > 	MainContext::get_default ()
		{
			if (!default_created) {
				default_context = std::shared_ptr<MainContext>(new MainContext);
				default_context->id_counter = 0;
				default_context->during_iteration = false;
				default_created = true;
			}
			return default_context;
		}

		// replaces signal_io::connect()
		sigc::connection MainContext::connect(sigc::slot<bool, IOCondition> slot, int fd, IOCondition condition, Priority priority)
		{
			//std::cerr << "MainContext::connect IO" << std::endl;
			struct pollfd pfd = {
				.fd = fd,
				.events = static_cast<short>(condition),
				.revents = static_cast<short>(0)};
			if(during_iteration) {
				added_signal_io_pfds.push_back(pfd);
				added_signal_io_slots.push_back(slot);
			} else {
				signal_io_pfds.push_back(pfd);
				signal_io_slots.push_back(slot);
			}
			return sigc::connection(slot);
		}
		// replaces signal_timeout::connect()
		sigc::connection MainContext::connect(sigc::slot<bool> slot, unsigned interval, Priority priority)
		{
			if (during_iteration) {
				//std::cerr << "MainContext::connect timeout during_iteration" << std::endl;
				added_signal_timeouts.push_back( Timeout( interval,std::shared_ptr<sigc::slot<bool> >( new sigc::slot<bool>(slot) ) ) );
				return added_signal_timeouts.back().connection;
			} else {
				//std::cerr << "MainContext::connect timeout not during_iteration" << std::endl;
				signal_timeouts.push_back( Timeout( interval,std::shared_ptr<sigc::slot<bool> >( new sigc::slot<bool>(slot) ) ) );
				return signal_timeouts.back().connection;
			}
			return sigc::connection(slot);
		}



		MainContext& signal_io() {
			return MainContext::get_default()->self();
		}

		MainContext& signal_timeout() {
			return MainContext::get_default()->self();
		}

} // namespace Slib
