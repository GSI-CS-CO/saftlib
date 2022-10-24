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
#ifndef SLIB_SOURCE_H
#define SLIB_SOURCE_H

#include <sigc++/sigc++.h>
#include <cstdint>
#include <vector>
#include <memory>
#include "PollFD.h"

namespace Slib
{

	class MainContext;

	class Source
	{
	public:
		unsigned int 	attach (const std::shared_ptr< MainContext >& context, std::shared_ptr<Source> self); // 	Adds a Source to a context so that it will be executed within that context.
		unsigned int 	get_id () const; // 	Returns the numeric ID for a particular source.
		std::shared_ptr< MainContext > 	get_context (); // 	Gets the MainContext with which the source is associated.
		void 	reference () const;
		void 	unreference () const;
		virtual 	~Source ();
	protected:
	 	Source ();
		sigc::connection 	connect_generic (const sigc::slot_base& slot);
		void 	add_poll (PollFD& poll_fd);
	 	//Adds a file descriptor to the set of file descriptors polled for this source.
		void 	remove_poll (PollFD& poll_fd);
	 	//Removes a file descriptor from the set of file descriptors polled for this source.
	 	//Gets the "current time" to be used when checking this source.
		int64_t get_time() const;
		virtual bool 	prepare (int& timeout)=0;
		virtual bool 	check ()=0;
		virtual bool 	dispatch (sigc::slot_base* slot)=0;

	private:
		std::shared_ptr<MainContext> _context;
		std::vector<PollFD*> _pfds;
		unsigned _id;
		friend class MainContext;

		sigc::slot_base _slot;
	};



} // namespace Slib

#endif