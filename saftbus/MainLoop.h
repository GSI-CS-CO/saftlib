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
