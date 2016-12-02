/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <assert.h>

#include "RegisteredObject.h"
#include "MasterFunctionGenerator.h"
#include "TimingReceiver.h"
#include "fg_regs.h"
#include "clog.h"

#include <thread>
#include <future>

// todo: minimize property-change signals to reduce dbus traffic


// currently testing: refprt -> shared_ptr - threads


namespace saftlib {

MasterFunctionGenerator::MasterFunctionGenerator(const ConstructorType& args)
 : Owned(args.objectPath),
   dev(args.dev),
   functionGenerators(args.functionGenerators)      
{
		for (auto fg : functionGenerators)
		{
			fg->signal_running.connect(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_running));
			fg->signal_armed.connect(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_armed));
			fg->signal_enabled.connect(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_enabled));
			fg->signal_started.connect(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_started));
			fg->signal_stopped.connect(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_stopped));

		}
		enabled=getEnabled();
}

MasterFunctionGenerator::~MasterFunctionGenerator()
{

}

// aggregate sigc signals from impl and forward via dbus where necessary

void MasterFunctionGenerator::on_fg_running(bool b)
{
	
}

void MasterFunctionGenerator::on_fg_armed(bool b)
{

}

void MasterFunctionGenerator::on_fg_enabled(bool b)
{
	// enabled true if any fg is running
	bool new_enabled = getEnabled();
	
	// signal only on change
	if (new_enabled != enabled)
	{
		enabled = new_enabled;
		Enabled(enabled);
	}
}

void MasterFunctionGenerator::on_fg_started(guint64 time)
{

}

void MasterFunctionGenerator::on_fg_stopped(guint64 time, bool abort, bool hardwareUnderflow, bool microcontrollerUnderflow)
{

}

Glib::RefPtr<MasterFunctionGenerator> MasterFunctionGenerator::create(const ConstructorType& args)
{
  return RegisteredObject<MasterFunctionGenerator>::create(args.objectPath, args);
}

bool MasterFunctionGenerator::AppendParameterSets(
	const std::vector< std::vector< gint16 > >& coeff_a, 
	const std::vector< std::vector< gint16 > >& coeff_b, 
	const std::vector< std::vector< gint32 > >& coeff_c, 
	const std::vector< std::vector< unsigned char > >& step, 
	const std::vector< std::vector< unsigned char > >& freq, 
	const std::vector< std::vector< unsigned char > >& shift_a, 
	const std::vector< std::vector< unsigned char > >& shift_b, 
	bool arm)
{

  // confirm equal number of FGs
  unsigned fgcount = coeff_a.size();
  if (coeff_b.size() != fgcount) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "coeff_b fgcount mismatch");
  if (coeff_c.size() != fgcount) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "coeff_c fgcount mismatch");
  if (step.size()    != fgcount) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "step fgcount mismatch");
  if (freq.size()    != fgcount) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "freq fgcount mismatch");
  if (shift_a.size() != fgcount) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_a fgcount mismatch");
  if (shift_b.size() != fgcount) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_b fgcount mismatch");


	if (fgcount > functionGenerators.size()) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "More datasets than function generators");	
	
	for (std::size_t i=0;i<fgcount;++i)
	{
		if (coeff_a[i].size()>0)
		{
			functionGenerators[i]->appendParameterSet(coeff_a[i], coeff_b[i], coeff_c[i], step[i], freq[i], shift_a[i], shift_b[i]);
		}
	}


	// Testing - send all datasets to 2 fgs, arm, abort all but last 2


// arming touches hardware - do not thread
/*
	std::vector<std::thread> loadthreads(fgcount);
	std::vector<std::future<bool>> futures(fgcount);
	for (std::size_t i=0;i<fgcount;i+=2)
	{
		futures[i] = std::async(std::launch::deferred, &FunctionGenerator::appendParameterSet,functionGenerators[i%200],coeff_a[i], coeff_b[i], coeff_c[i], step[i], freq[i], shift_a[i], shift_b[i], arm);
		futures[i+1] = std::async(std::launch::deferred, &FunctionGenerator::appendParameterSet,functionGenerators[(i+1)%200],coeff_a[i+1], coeff_b[i+1], coeff_c[i+1], step[i+1], freq[i+1], shift_a[i+1], shift_b[i+1],arm);
		

		loadthreads[i] = std::thread(&FunctionGenerator::appendParameterSet,functionGenerators[i%2],coeff_a[i], coeff_b[i], coeff_c[i], step[i], freq[i], shift_a[i], shift_b[i], arm);
		loadthreads[i+1] = std::thread(&FunctionGenerator::appendParameterSet,functionGenerators[(i+1)%2],coeff_a[i+1], coeff_b[i+1], coeff_c[i+1], step[i+1], freq[i+1], shift_a[i+1], shift_b[i+1], arm);
		loadthreads[i].join();
		loadthreads[i+1].join();
		
	}
	
		for (std::size_t i=0;i<fgcount;i+=2)
		{
			futures[i].get();
		}
	*/
	/*
	for (std::size_t i=0;i<fgcount;++i)
	{
		//bool lowfill = functionGenerators[i%2]->appendParameterSet(coeff_a[i], coeff_b[i], coeff_c[i], step[i], freq[i], shift_a[i], shift_b[i], arm);	

		loadthreads[i] = std::thread(&FunctionGenerator::appendParameterSet,functionGenerators[i%2],coeff_a[i], coeff_b[i], coeff_c[i], step[i], freq[i], shift_a[i], shift_b[i], arm);
	}
	
	for (auto& t : loadthreads)
	{
		t.join();
	}
*/

// todo: check if data has been sent



	
	if (arm)
	{
	
  Glib::RefPtr<Glib::MainLoop>    mainloop = Glib::MainLoop::create();
  Glib::RefPtr<Glib::MainContext> context  = mainloop->get_context();

	for (std::size_t i=0;i<fgcount;++i)
	{
		if (arm && coeff_a[i].size() > 0)
		{
			functionGenerators[i]->arm();
			sleep(1); // ?? crashing with > 7 fgs simultaneously? seems to fix. No it crashed again.
			// allow interrupts through? Still crashes.
			context->iteration(true);
		}		
	}

	// wait for arm response ...

	bool all_armed=false;
	do
	{
		all_armed=true;
		/*
		for (auto fg : functionGenerators)
		{
			all_armed &= fg->armed;
		}
		*/
		
		for (std::size_t i=0;i<fgcount;++i)
		{
			if (coeff_a[i].size()==0) continue;				
			all_armed &= functionGenerators[i]->armed;
		}
		
		// allow arm interrupts through
		context->iteration(false);
	} while (all_armed == false) ;
//		Stopped(1000,false,false,false);	
	}
	return false;
}


void MasterFunctionGenerator::Flush()
{
  ownerOnly();
	for (auto&& fg : functionGenerators)
	{		
		fg->flush();
	}

/*  
 	for (auto fg : functionGenerators)
	{
		if (fg->getEnabled())
		{
		    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "SUb-FG Enabled, cannot Flush");
		}
	}
*/

}


guint32 MasterFunctionGenerator::getStartTag() const
{
  return startTag;
}


void MasterFunctionGenerator::Arm()
{
  ownerOnly();
	for (auto fg : functionGenerators)
	{
		try {
			fg->arm();
		}	
		catch (Gio::DBus::Error& ex)
		{
		}
	}
	
	return;
	
	// wait for arm response ...
/*	
  Glib::RefPtr<Glib::MainLoop>    mainloop = Glib::MainLoop::create();
  Glib::RefPtr<Glib::MainContext> context  = mainloop->get_context();

	
	bool all_armed=false;
	do
	{
		all_armed=true;
		for (auto fg : functionGenerators)
		{
			all_armed &= fg->armed;
		}
		
		// allow arm interrupts 
		context->iteration(true);
	} while (all_armed == false) ;
	*/	
	
}


void MasterFunctionGenerator::Reset()
{
	for (auto fg : functionGenerators)
	{
		fg->Reset();
	}
}

void MasterFunctionGenerator::Abort()
{
  ownerOnly();
  Reset();
  
  Glib::RefPtr<Glib::MainLoop>    mainloop = Glib::MainLoop::create();
  Glib::RefPtr<Glib::MainContext> context  = mainloop->get_context();

	bool all_stopped=false;
	do
	{
		all_stopped=true;
		for (auto fg : functionGenerators)
		{
			all_stopped &= !fg->running;
		}
		
		// allow interrupts to be processed
		context->iteration(false);
	} while (all_stopped == false) ;

}

void MasterFunctionGenerator::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off all the function generators
  Reset();
}

void MasterFunctionGenerator::setStartTag(guint32 val)
{
  ownerOnly();

 	for (auto fg : functionGenerators)  
 	{
    if (fg->enabled)
	    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "FG Enabled, cannot set StartTag");
	}
  
  if (val != startTag) {
    startTag = val;
  	for (auto fg : functionGenerators)
		{
			fg->startTag=startTag;
		}

    StartTag(startTag);
  }
}

// at least 1 FG is enabled
bool MasterFunctionGenerator::getEnabled() const
{
bool any_enabled=false;
	for (auto fg : functionGenerators)
	{
		any_enabled |= fg->enabled;	
	}

	return any_enabled;
}

std::vector<guint32> MasterFunctionGenerator::ReadExecutedParameterCounts()
{
	std::vector<guint32> counts;
	for (auto fg : functionGenerators)
	{
		counts.push_back(fg->executedParameterCount);
	}
	return counts;
}

std::vector<guint64> MasterFunctionGenerator::ReadFillLevels()
{
	std::vector<guint64> levels;
	for (auto fg : functionGenerators)
	{
		levels.push_back(fg->ReadFillLevel());
	}
	return levels;
}

std::vector<Glib::ustring> MasterFunctionGenerator::ReadNames()
{
	std::vector<Glib::ustring> names;
	for (auto fg : functionGenerators)
	{
    std::ostringstream ss;
    ss << "fg-" << (int)fg->getSCUbusSlot() << "-" << (int)fg->getDeviceNumber();
    ss << "(" << fg->index << ")";
    Glib::ustring name = ss.str();	
		names.push_back(name);
	}
	return names;
}

}
