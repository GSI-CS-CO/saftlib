#include "Source.h"
#include "MainContext.h"
#include <time.h>

#include <iostream>

namespace Slib
{

	// 	Adds a Source to a context so that it will be executed within that context.
	unsigned int Source::attach (const std::shared_ptr< MainContext >& context, std::shared_ptr<Source> self) 
	{
		//std::cerr << "Source::attach " << std::endl;
		_id      = ++context->id_counter;
		//std::cerr << "_id = " << _id << std::endl;
		context->sources[_id] = self; // undefined behavior
		_context = context;
		return _id;
	} 
	// 	Returns the numeric ID for a particular source.
	unsigned int Source::get_id () const
	{
		return _id;
	} 
	// 	Gets the MainContext with which the source is associated.
	std::shared_ptr< MainContext > Source::get_context () 
	{
		return _context;
	} 


	// const GSource* 	gobj () const;
	// GSource* 	gobj_copy () const;
	void Source::reference () const
	{
		//nothing
	}
	void Source::unreference () const
	{
		//nothing
	}

 	//Construct an object that uses the virtual functions prepare(), check() and dispatch().
 	Source::Source () 
 	{

 	}
	sigc::connection Source::connect_generic (const sigc::slot_base& slot) 
	{
		_slot = slot;
		return sigc::connection(_slot);
	}
	Source::~Source () 
	{

	}
 	//Adds a file descriptor to the set of file descriptors polled for this source.
	void Source::add_poll (PollFD& poll_fd) 
	{
		_pfds.push_back(&poll_fd);
	}

 	//Removes a file descriptor from the set of file descriptors polled for this source.
	void Source::remove_poll (PollFD& poll_fd) 
	{
		std::vector<PollFD*> new_pdfs;
		for(unsigned i = 0; i < _pfds.size(); ++i) {
			if (!(poll_fd.pfd.fd == _pfds[i]->pfd.fd)) {
				new_pdfs.push_back(_pfds[i]);
			}
		}
		_pfds = new_pdfs;
	}
 	//Gets the "current time" to be used when checking this source.
 	// returns time in microseconds
	int64_t Source::get_time() const
	{
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		return now.tv_sec  * 1000000 + 
		       now.tv_nsec / 1000;
	}






} // namespace Slib
