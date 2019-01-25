
#include "MainContext.h"
#include "Source.h"

#include <algorithm>
#include <iostream>

namespace Slib
{

		std::shared_ptr< MainContext > MainContext::default_context;
		bool MainContext::default_created = false;

		MainContext& MainContext::self() {
			return *this;
		}


		 // 	Runs a single iteration for the given main loop.
		bool MainContext::iteration (bool may_block)
		{
			// was hier passieren muss:
			// - alle fds einsammeln und in ein Array stopfen
			// - poll() aufrufen
			// - Ergebnisse an sources zurueckliefern

			during_iteration = true;
			std::cerr << "MainContext::iteration()" << std::endl;

			// poll all fds from all sources
			bool result = false;
			std::vector<struct pollfd> all_pfds(signal_io_pfds.begin(), signal_io_pfds.end()); // copy all signal_io_pfds
			std::vector<PollFD*>       all_pfds_ptr;
			std::vector<unsigned> all_ids;
			int timeout_ms = -1;
			for(auto &id_source: sources) {
				auto id     = id_source.first;
				auto source = id_source.second;
				int  source_timeout_ms = -1;

				if (source->prepare(source_timeout_ms)) {
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
			std::cerr << "MainContext::iteration   -> poll()" << std::endl;
			std::vector<int> signal_io_removed_indices;
			if ((result = poll(&all_pfds[0], all_pfds.size(), timeout_ms)) > 0) {
				std::cerr << "MainContext::iteration   -> poll done" << std::endl;
				int idx = 0;
				for (auto fd: all_pfds) {
					if (idx < signal_io_pfds.size()) {
						// copy back
						signal_io_pfds[idx] = fd;
						if (fd.events & fd.revents) {
							//execute  signal_io callback
							bool result = signal_io_slots[idx](fd.revents);
							if (result == false ) { // add this index to the removal list
								std::cerr << "signal_io " << idx << " callback returned false" << std::endl;
								signal_io_removed_indices.push_back(idx);
							}
						}
					} else {
						// copy the results back into the PollFD 
						all_pfds_ptr[idx-signal_io_pfds.size()]->pfd.revents = all_pfds[idx].revents;
					}

					++idx;
				}
			}

			for (auto &id_source: sources) {
				auto id     = id_source.first;
				auto source = id_source.second;
				std::cerr << "checking source id " << id << std::endl;
				if (source->check()) {
					std::cerr << "dispatching source id " << id << std::endl;
					source->dispatch(&source->_slot);
				}
			}


			// remove all signal_ios whose callbacks returned false
			if (signal_io_removed_indices.size() > 0) {
				std::vector<struct pollfd>                  new_signal_io_pfds;
				std::vector<sigc::slot<bool, IOCondition> > new_signal_io_slots;
				for (int i = 0; i < signal_io_pfds.size(); ++i) {
					bool found_in_removal_list = false;
					for (int n = 0; n < signal_io_removed_indices.size(); ++n) {
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

			// add the newly created signal_ios
			if (added_signal_io_pfds.size() > 0) {
				for (int i = 0; i < added_signal_io_pfds.size(); ++i) {
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

		sigc::connection MainContext::connect(sigc::slot<bool, IOCondition> slot, int fd, IOCondition condition, Priority priority)
		{
			std::cerr << "MainContext::connect" << std::endl;
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

		MainContext& signal_io() {
			return MainContext::get_default()->self();
		}


} // namespace Slib
