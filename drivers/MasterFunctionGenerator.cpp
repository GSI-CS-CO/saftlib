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



namespace saftlib {

MasterFunctionGenerator::MasterFunctionGenerator(const ConstructorType& args)
 : Owned(args.objectPath),
   dev(args.dev),
   allFunctionGenerators(args.functionGenerators),
   activeFunctionGenerators(args.functionGenerators),
   generateIndividualSignals(false)   
{
  for (auto fg : allFunctionGenerators)
  {
    fg->signal_running.connect(sigc::bind<0>(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_running),fg)); 
    fg->signal_armed.connect(sigc::bind<0>(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_armed),fg)); 
    fg->signal_enabled.connect(sigc::bind<0>(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_enabled),fg)); 
    fg->signal_started.connect(sigc::bind<0>(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_started),fg)); 
    fg->signal_stopped.connect(sigc::bind<0>(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_stopped),fg)); 
    fg->signal_refill.connect(sigc::bind<0>(sigc::mem_fun(*this, &MasterFunctionGenerator::on_fg_refill),fg)); 
  }
}

MasterFunctionGenerator::~MasterFunctionGenerator()
{
      
}

// aggregate sigc signals from impl and forward via dbus where necessary

void MasterFunctionGenerator::on_fg_running(std::shared_ptr<FunctionGeneratorImpl>& fg, bool running)
{
  if (generateIndividualSignals && std::find(activeFunctionGenerators.begin(),activeFunctionGenerators.end(),fg)!=activeFunctionGenerators.end())
  {
    Running(fg->GetName(), running);
  }
}

void MasterFunctionGenerator::on_fg_refill(std::shared_ptr<FunctionGeneratorImpl>& fg)
{
  if (generateIndividualSignals && std::find(activeFunctionGenerators.begin(),activeFunctionGenerators.end(),fg)!=activeFunctionGenerators.end())
  {
    Refill(fg->GetName());
  }
}
// watches armed notifications of individual FGs
// sends AllArmed signal when all fgs with data have signaled armed(true)
void MasterFunctionGenerator::on_fg_armed(std::shared_ptr<FunctionGeneratorImpl>& fg, bool armed)
{

  if (generateIndividualSignals)
  {
    Armed(fg->GetName(), armed);
  }
  //clog << "FG Armed  TID: " << syscall(SYS_gettid) << std::endl;
  if (armed)
  {
    bool all_armed=true;
    for (auto fg : activeFunctionGenerators)
    {
      bool fg_armed_or_inactive = fg->getArmed() || (fg->ReadFillLevel()==0);
      all_armed &= fg_armed_or_inactive;
    }
    if (all_armed)
    {
      AllArmed();
    }
  }
}

void MasterFunctionGenerator::on_fg_enabled(std::shared_ptr<FunctionGeneratorImpl>& fg, bool enabled)
{
  if (generateIndividualSignals)
  {
    Enabled(fg->GetName(), enabled);
  }
}

void MasterFunctionGenerator::on_fg_started(std::shared_ptr<FunctionGeneratorImpl>& fg, guint64 time)
{

  if (generateIndividualSignals)
  {
    Started(fg->GetName(), time);
  }
}

// Forward Stopped signal 
void MasterFunctionGenerator::on_fg_stopped(std::shared_ptr<FunctionGeneratorImpl>& fg, guint64 time, bool abort, bool hardwareUnderflow, bool microcontrollerUnderflow)
{
  if (generateIndividualSignals)
  {
    Stopped(fg->GetName(), time, abort, hardwareUnderflow, microcontrollerUnderflow);
  }

	bool all_stopped=true;
  for (auto fg : activeFunctionGenerators)
	{
    all_stopped &= !fg->getRunning();
	}
  if (all_stopped)
  {
    AllStopped(time);
  }
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
	bool arm,
  bool wait_for_arm_ack)
{

  ownerOnly();

  // confirm equal number of FGs
  unsigned fgcount = coeff_a.size();
  if (coeff_b.size() != fgcount) throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "coeff_b fgcount mismatch");
  if (coeff_c.size() != fgcount) throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "coeff_c fgcount mismatch");
  if (step.size()    != fgcount) throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "step fgcount mismatch");
  if (freq.size()    != fgcount) throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "freq fgcount mismatch");
  if (shift_a.size() != fgcount) throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "shift_a fgcount mismatch");
  if (shift_b.size() != fgcount) throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "shift_b fgcount mismatch");


	if (fgcount > activeFunctionGenerators.size()) throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "More datasets than function generators");	

  bool lowFill=false;
	for (std::size_t i=0;i<fgcount;++i)
	{
		if (coeff_a[i].size()>0)
		{
			lowFill |= activeFunctionGenerators[i]->appendParameterSet(coeff_a[i], coeff_b[i], coeff_c[i], step[i], freq[i], shift_a[i], shift_b[i]);
		}
	}

	// if requested wait for all fgs to arm
	if (arm)
  {
    arm_all();
    // wait for arm response ...
    if (wait_for_arm_ack)
    {
      waitForCondition(std::bind(&MasterFunctionGenerator::all_armed, this), 2000);
    }
  }
  return lowFill;
}


void MasterFunctionGenerator::Flush()
{
  std::string error_msg;
  ownerOnly();
	for (auto fg : activeFunctionGenerators)
	{
		try
    {
      fg->flush();      
		}	
		catch (IPC_METHOD::Error& ex)
		{
      error_msg += (fg->GetName() + ex.what());
		}
	}
  if (!error_msg.empty())
  {
      throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, error_msg);
  }
}


guint32 MasterFunctionGenerator::getStartTag() const
{
  return startTag;
}

void MasterFunctionGenerator::setGenerateIndividualSignals(bool newvalue)
{
  generateIndividualSignals=newvalue;
}

bool MasterFunctionGenerator::getGenerateIndividualSignals() const
{
  return generateIndividualSignals;
}


void MasterFunctionGenerator::arm_all()
{
  std::string error_msg;
	for (auto fg : activeFunctionGenerators)
	{
		try
    {
      if (fg->fillLevel>0)
      {
			  fg->arm();
      }
		}	
		catch (IPC_METHOD::Error& ex)
		{
      error_msg += (fg->GetName() + ex.what());
		}
	}
  if (!error_msg.empty())
  {
      throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, error_msg);
  }
}


void MasterFunctionGenerator::Arm()
{
  ownerOnly();
  arm_all();
}


void MasterFunctionGenerator::reset_all()
{
	for (auto fg : activeFunctionGenerators)
	{
		fg->Reset();
	}
}

void MasterFunctionGenerator::Abort(bool wait_for_abort_ack)
{
  ownerOnly();
  reset_all();
  if (wait_for_abort_ack)
  {
    waitForCondition(std::bind(&MasterFunctionGenerator::all_stopped, this), 2000);
  }
}

void MasterFunctionGenerator::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off all the function generators
  reset_all();
  activeFunctionGenerators = allFunctionGenerators;
}

void MasterFunctionGenerator::setStartTag(guint32 val)
{
  ownerOnly();

 	for (auto fg : activeFunctionGenerators)  
 	{
    if (fg->enabled)
	    throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "FG Enabled, cannot set StartTag");
	}
  
  if (val != startTag) {
    startTag = val;
  	for (auto fg : activeFunctionGenerators)
		{
			fg->startTag=startTag;
		}

    StartTag(startTag);
  }
}


std::vector<guint32> MasterFunctionGenerator::ReadExecutedParameterCounts()
{
	std::vector<guint32> counts;
	for (auto fg : activeFunctionGenerators)
	{
		counts.push_back(fg->executedParameterCount);
	}
	return counts;
}

std::vector<guint64> MasterFunctionGenerator::ReadFillLevels()
{
	std::vector<guint64> levels;
	for (auto fg : activeFunctionGenerators)
	{
		levels.push_back(fg->ReadFillLevel());
	}
	return levels;
}

std::vector<Glib::ustring> MasterFunctionGenerator::ReadAllNames()
{
	std::vector<Glib::ustring> names;
	for (auto fg : allFunctionGenerators)
	{
		names.push_back(fg->GetName());
	}
	return names;
}

std::vector<Glib::ustring> MasterFunctionGenerator::ReadNames()
{
	std::vector<Glib::ustring> names;
	for (auto fg : activeFunctionGenerators)
	{
		names.push_back(fg->GetName());
	}
	return names;
}

// vector<bool> as used in glib 2.50 requires c++14 
std::vector<int> MasterFunctionGenerator::ReadArmed()
{
	std::vector<int> armed_states;
	for (auto fg : activeFunctionGenerators)
	{
	  armed_states.push_back(fg->getArmed() ? 1 : 0);
	}
	return armed_states;
}

std::vector<int> MasterFunctionGenerator::ReadEnabled()
{
	std::vector<int> enabled_states;
	for (auto fg : activeFunctionGenerators)
	{
	  enabled_states.push_back(fg->getEnabled() ? 1 : 0);
	}
	return enabled_states;
}

std::vector<int> MasterFunctionGenerator::ReadRunning()
{
	std::vector<int> running_states;
	for (auto fg : activeFunctionGenerators)
	{
	  running_states.push_back(fg->getRunning() ? 1 : 0);
	}
	return running_states;
}

void MasterFunctionGenerator::ResetActiveFunctionGenerators()
{
  ownerOnly();
  activeFunctionGenerators = allFunctionGenerators;
  generateIndividualSignals=false;
}

void MasterFunctionGenerator::SetActiveFunctionGenerators(const std::vector<Glib::ustring>& names)
{
  ownerOnly();
  if (names.size()==0)
  {
    throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "No Function Generators Selected" );
  }


  for (auto name : names)
  {
    if (std::any_of(allFunctionGenerators.begin(), allFunctionGenerators.end(),
          [name](std::shared_ptr<FunctionGeneratorImpl> fg){ return name==fg->GetName();}) == false)
    {
      throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS, "Function Generator Not Found " + name);
    }
  }

  activeFunctionGenerators.clear();

  for (auto name : names)
  {
    auto fg_it = std::find_if(allFunctionGenerators.begin(), allFunctionGenerators.end(),
          [name](std::shared_ptr<FunctionGeneratorImpl> fg){ return name==fg->GetName();});
    activeFunctionGenerators.push_back(*fg_it);
  }
}


bool MasterFunctionGenerator::WaitTimeout()
{
  clog << "MasterFG Timed out waiting" << std::endl;
  waitTimeout.disconnect();
  return false;
}

bool MasterFunctionGenerator::all_armed()
{
  bool all_armed=true;
  for (auto fg : activeFunctionGenerators)
  {
    if (fg->fillLevel>0)
      all_armed &= fg->armed;
  }
  return all_armed;
}

bool MasterFunctionGenerator::all_stopped()
{
  bool all_stopped=false;
  all_stopped=true;
  for (auto fg : activeFunctionGenerators)
  {
    all_stopped &= !fg->running;
  }
  return all_stopped; 
}

void MasterFunctionGenerator::waitForCondition(std::function<bool()> condition, int timeout_ms)
{
  if (waitTimeout.connected()) {
    throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS,"Waiting for armed: Timeout already active");
  }
  waitTimeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &MasterFunctionGenerator::WaitTimeout),timeout_ms);

  Glib::RefPtr<Glib::MainLoop>    mainloop = Glib::MainLoop::create();
  Glib::RefPtr<Glib::MainContext> context  = mainloop->get_context();
  do
  {
    context->iteration(false);
    if (!waitTimeout.connected()) {
      throw IPC_METHOD::Error(IPC_METHOD::Error::INVALID_ARGS,"MasterFG: Timeout waiting for arm acknowledgements");
    }
  } while (condition() == false) ;
  waitTimeout.disconnect();
}



}
