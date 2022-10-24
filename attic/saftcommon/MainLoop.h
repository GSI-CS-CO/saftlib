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
#ifndef SLIB_MAIN_LOOP_H
#define SLIB_MAIN_LOOP_H

#include <sigc++/sigc++.h>
#include <cstdint>
#include <memory>

#include "MainContext.h"

namespace Slib
{

	class MainLoop
	{
	public:
 		void run(); //	Runs a main loop until quit() is called on the loop.
		void quit();//  Stops a MainLoop from running.
		bool is_running();//	Checks to see if the main loop is currently being run via run().
		std::shared_ptr< MainContext > 	get_context (); // Returns the MainContext of loop.
		void reference() const; //	Increases the reference count on a MainLoop object by one.
		void unreference() const; //Decreases the reference count on a MainLoop object by one.
		// GMainLoop* 	gobj ();
		// const GMainLoop* 	gobj () const;
		// GMainLoop* 	gobj_copy () const;
		//Static Public Member Functions;
		static std::shared_ptr< MainLoop >	create (bool is_running=false);
		//static std::shared_ptr< MainLoop >	create (const std::shared_ptr< MainContext >& context/*, bool is_running=false*/);
		static int 	depth ()	;
	private:
		bool running;
		std::shared_ptr< MainContext> context;
	};




} // namespace Slib

#endif
