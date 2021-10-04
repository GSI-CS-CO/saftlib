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
#include <algorithm>
#include <time.h>

#include "RegisteredObject.h"
#include "MasterFunctionGenerator.h"
#include "TimingReceiver.h"
#include "fg_regs.h"
#include "clog.h"



namespace saftlib {

MasterFunctionGenerator::MasterFunctionGenerator(const ConstructorType& args)
 : Owned(args.objectPath),
   tr(args.tr),
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
  for (auto fg : allFunctionGenerators) {
    fg->signal_running.clear();
    fg->signal_armed.clear();
    fg->signal_enabled.clear();
    fg->signal_started.clear();
    fg->signal_stopped.clear();
    fg->signal_refill.clear();
  }
  allFunctionGenerators.clear();
  activeFunctionGenerators.clear();
}

// aggregate sigc signals from impl and forward via dbus where necessary

void MasterFunctionGenerator::on_fg_running(std::shared_ptr<FunctionGeneratorImpl>& fg, bool running)
{
  DRIVER_LOG("",-1,running);
  if (generateIndividualSignals && std::find(activeFunctionGenerators.begin(),activeFunctionGenerators.end(),fg)!=activeFunctionGenerators.end())
  {
    Running(fg->GetName(), running);
  }
}

void MasterFunctionGenerator::on_fg_refill(std::shared_ptr<FunctionGeneratorImpl>& fg)
{
  DRIVER_LOG("",-1,-1);
  if (generateIndividualSignals && std::find(activeFunctionGenerators.begin(),activeFunctionGenerators.end(),fg)!=activeFunctionGenerators.end())
  {
    Refill(fg->GetName());
  }
}
// watches armed notifications of individual FGs
// sends AllArmed signal when all fgs with data have signaled armed(true)
void MasterFunctionGenerator::on_fg_armed(std::shared_ptr<FunctionGeneratorImpl>& fg, bool armed)
{

  DRIVER_LOG("",-1,armed);
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
      DRIVER_LOG("AllArmed",-1,-1);
      AllArmed();
    }
  }
}

void MasterFunctionGenerator::on_fg_enabled(std::shared_ptr<FunctionGeneratorImpl>& fg, bool enabled)
{
  DRIVER_LOG("",-1,-1);
  if (generateIndividualSignals)
  {
    Enabled(fg->GetName(), enabled);
  }
}

void MasterFunctionGenerator::on_fg_started(std::shared_ptr<FunctionGeneratorImpl>& fg, uint64_t time)
{

  DRIVER_LOG("",-1,-1);
  if (generateIndividualSignals)
  {
    SigStarted(fg->GetName(), saftlib::makeTimeTAI(time));
  }
}

// Forward Stopped signal 
void MasterFunctionGenerator::on_fg_stopped(std::shared_ptr<FunctionGeneratorImpl>& fg, uint64_t time, bool abort, bool hardwareUnderflow, bool microcontrollerUnderflow)
{
  DRIVER_LOG("",-1,-1);
  if (generateIndividualSignals)
  {
    SigStopped(fg->GetName(), saftlib::makeTimeTAI(time), abort, hardwareUnderflow, microcontrollerUnderflow);
  }

	bool all_stopped=true;
  for (auto fg : activeFunctionGenerators)
	{
    all_stopped &= !fg->getRunning();
	}
  if (all_stopped)
  {
    DRIVER_LOG("all_stopped",-1,-1);
    SigAllStopped(saftlib::makeTimeTAI(time));
  }
}

std::shared_ptr<MasterFunctionGenerator> MasterFunctionGenerator::create(const ConstructorType& args)
{
  return RegisteredObject<MasterFunctionGenerator>::create(args.objectPath, args);
}

void MasterFunctionGenerator::InitializeSharedMemory(const std::string& shared_memory_name)
{
  try 
  {
    shm_params.reset( new boost::interprocess::managed_shared_memory(boost::interprocess::open_only, shared_memory_name.c_str()));
  } 
  catch (boost::interprocess::interprocess_exception& e)
  {
    std::ostringstream msg;
    msg << "Error initializing shared memory: " << e.what();
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, msg.str());
  }

  if (!shm_params->check_sanity())
  {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Shared memory is insane");
  }
  
  shm_mutex = (shm_params->find<boost::interprocess::interprocess_mutex>("mutex")).first;
  
  if (!shm_mutex)
  {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "No mutex in shared memory");
  }

  int* version = (shm_params->find<int32_t>("format-version")).first;
  int format_version=0;
  if (version) {
    format_version = *version;
    //std::cout << "Shared memory format version " << *version << std::endl;
  } else {
    //std::cout << "Shared memory format version not found" << std::endl;
  }
  if (format_version < 0 || format_version > 0)
  {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Unsupported shared memory format version");
  }
}


void MasterFunctionGenerator::AppendParameterTuplesForBeamProcess(int beam_process, bool arm, bool wait_for_arm_ack)
{

  DRIVER_LOG("",-1,-1);
  if (!shm_params)
  {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Shared memory not initialized");
  }

  if (!shm_mutex)
  {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "No mutex in shared memory");
  }

  try {

    if (!shm_params->check_sanity())
    {
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Shared memory is insane");
    }

    {
      boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*shm_mutex);
      typedef std::pair<int,int> KeyType;
      typedef ParameterVector ValueType;
      typedef std::pair<const KeyType, ValueType> MapEntryType;
      typedef boost::interprocess::allocator<MapEntryType,boost::interprocess::managed_shared_memory::segment_manager> MapAllocator;
      typedef boost::interprocess::map<KeyType, ValueType, std::less<KeyType>, MapAllocator> IndexMap;

      IndexMap* indexMap = shm_params->find<IndexMap>("IndexMap").first;
//      std::cout << "map size " << indexMap->size() << std::endl;
      KeyType key;
      key.first=0;
      key.second=beam_process;
      for (auto fg : activeFunctionGenerators)
      {
        /* Layout V0.1
        std::pair<ParameterVector *, std::size_t> fred = shm_params->find<ParameterVector> (activeFunctionGenerators[0]->GetName().c_str());
        ParameterVector* paramVector = fred.first;

        if ( paramVector)
        {
          std::cout << fg->GetName() << " found in shared memory" << std::endl;
          std::cout << "Parameter Tuples: " <<  paramVector->size() << std::endl;
          std::cout << (*paramVector)[0].coeff_a << std::endl;
          std::cout << (*paramVector)[0].coeff_b << std::endl;
          std::cout << (*paramVector)[0].coeff_c << std::endl;
          fg->appendParameterTuples(paramVector->cbegin(), paramVector->cend());

        } else {
          throw saftbus::Error(saftbus::Error::INVALID_ARGS, "No parameter vector in shared memory");
        }
        */

        if (indexMap->find(key) != indexMap->end()) {
          ParameterVector& v = indexMap->at(key);
//          std::cout << key.first << "," << key.second <<  " found in shared memory ";
//          std::cout << "Parameter Tuples: " <<  v.size() << std::endl;
          fg->appendParameterTuples(v.cbegin(), v.cend());
        }
        key.first++;
      } 
    } // end mutex scope

// if requested wait for all fgs to arm
  	if (arm)
    {
      DRIVER_LOG("arm",-1,-1);
      arm_all();
      if (wait_for_arm_ack)
      {
        DRIVER_LOG("wait_for_arm_ack",-1,-1);
        waitForCondition(std::bind(&MasterFunctionGenerator::all_armed, this), 2000);
      }
    }
  } catch (boost::interprocess::interprocess_exception& e) {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, e.what());
  }
}


	
bool MasterFunctionGenerator::AppendParameterSets(
	const std::vector< std::vector< int16_t > >& coeff_a, 
	const std::vector< std::vector< int16_t > >& coeff_b, 
	const std::vector< std::vector< int32_t > >& coeff_c, 
	const std::vector< std::vector< unsigned char > >& step, 
	const std::vector< std::vector< unsigned char > >& freq, 
	const std::vector< std::vector< unsigned char > >& shift_a, 
	const std::vector< std::vector< unsigned char > >& shift_b, 
	bool arm,
  bool wait_for_arm_ack)
{

  DRIVER_LOG("",-1,coeff_a.size());
  ownerOnly();

  // confirm equal number of FGs
  unsigned fgcount = coeff_a.size();
  if (coeff_b.size() != fgcount) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "coeff_b fgcount mismatch");
  if (coeff_c.size() != fgcount) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "coeff_c fgcount mismatch");
  if (step.size()    != fgcount) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "step fgcount mismatch");
  if (freq.size()    != fgcount) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "freq fgcount mismatch");
  if (shift_a.size() != fgcount) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "shift_a fgcount mismatch");
  if (shift_b.size() != fgcount) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "shift_b fgcount mismatch");


	if (fgcount > activeFunctionGenerators.size()) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "More datasets than function generators");	

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
      DRIVER_LOG("arm",-1,-1);
    arm_all();
    // wait for arm response ...
    if (wait_for_arm_ack)
    {
        DRIVER_LOG("wait_for_arm_ack",-1,-1);
      waitForCondition(std::bind(&MasterFunctionGenerator::all_armed, this), 2000);
    }
  }
  return lowFill;
}


void MasterFunctionGenerator::Flush()
{
  DRIVER_LOG("",-1,-1);
  std::string error_msg;
  ownerOnly();
	for (auto fg : activeFunctionGenerators)
	{
		try
    {
      fg->flush();      
		}	
		catch (saftbus::Error& ex)
		{
      error_msg += (fg->GetName() + ex.what());
		}
	}
  if (!error_msg.empty())
  {
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, error_msg);
  }
}


uint32_t MasterFunctionGenerator::getStartTag() const
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
  DRIVER_LOG("",-1,-1);
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
		catch (saftbus::Error& ex)
		{
      error_msg += (fg->GetName() + ex.what());
		}
	}
  if (!error_msg.empty())
  {
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, error_msg);
  }
}


void MasterFunctionGenerator::Arm()
{
  DRIVER_LOG("",-1,-1);
  ownerOnly();
  arm_all();
}


void MasterFunctionGenerator::reset_all()
{
  DRIVER_LOG("",-1,-1);
	for (auto fg : activeFunctionGenerators)
	{
		fg->Reset();
	}
}

void MasterFunctionGenerator::Abort(bool wait_for_abort_ack)
{
  DRIVER_LOG("",-1,-1);
  ownerOnly();
  reset_all();
  sleep(5);
  // if (wait_for_abort_ack)
  // {
  //   waitForCondition(std::bind(&MasterFunctionGenerator::all_stopped, this), 2000);
  // }
}

void MasterFunctionGenerator::ownerQuit()
{
  DRIVER_LOG("",-1,-1);
  // owner quit without Disown? probably a crash => turn off all the function generators
  reset_all();
  activeFunctionGenerators = allFunctionGenerators;
}

void MasterFunctionGenerator::setStartTag(uint32_t val)
{
  DRIVER_LOG("",-1,-1);
  ownerOnly();

 	for (auto fg : activeFunctionGenerators)  
 	{
    if (fg->enabled)
	    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "FG Enabled, cannot set StartTag");
	}
  
  if (val != startTag) {
    startTag = val;
  	for (auto fg : activeFunctionGenerators)
		{
			fg->startTag=startTag;
		}
  }
}


std::vector<uint32_t> MasterFunctionGenerator::ReadExecutedParameterCounts()
{
  DRIVER_LOG("",-1,-1);
	std::vector<uint32_t> counts;
	for (auto fg : activeFunctionGenerators)
	{
		counts.push_back(fg->executedParameterCount);
	}
	return counts;
}

std::vector<uint64_t> MasterFunctionGenerator::ReadFillLevels()
{
  DRIVER_LOG("",-1,-1);
	std::vector<uint64_t> levels;
	for (auto fg : activeFunctionGenerators)
	{
		levels.push_back(fg->ReadFillLevel());
	}
	return levels;
}

std::vector<std::string> MasterFunctionGenerator::ReadAllNames()
{
  DRIVER_LOG("",-1,-1);
	std::vector<std::string> names;
	for (auto fg : allFunctionGenerators)
	{
		names.push_back(fg->GetName());
	}
	return names;
}

std::vector<std::string> MasterFunctionGenerator::ReadNames()
{
  DRIVER_LOG("",-1,-1);
	std::vector<std::string> names;
	for (auto fg : activeFunctionGenerators)
	{
		names.push_back(fg->GetName());
	}
	return names;
}

// vector<bool> as used in glib 2.50 requires c++14 
std::vector<int> MasterFunctionGenerator::ReadArmed()
{
  DRIVER_LOG("",-1,-1);
	std::vector<int> armed_states;
	for (auto fg : activeFunctionGenerators)
	{
	  armed_states.push_back(fg->getArmed() ? 1 : 0);
	}
	return armed_states;
}

std::vector<int> MasterFunctionGenerator::ReadEnabled()
{
  DRIVER_LOG("",-1,-1);
	std::vector<int> enabled_states;
	for (auto fg : activeFunctionGenerators)
	{
	  enabled_states.push_back(fg->getEnabled() ? 1 : 0);
	}
	return enabled_states;
}

std::vector<int> MasterFunctionGenerator::ReadRunning()
{
  DRIVER_LOG("",-1,-1);
	std::vector<int> running_states;
	for (auto fg : activeFunctionGenerators)
	{
	  running_states.push_back(fg->getRunning() ? 1 : 0);
	}
	return running_states;
}

void MasterFunctionGenerator::ResetActiveFunctionGenerators()
{
  DRIVER_LOG("",-1,-1);
  ownerOnly();
  activeFunctionGenerators = allFunctionGenerators;
  generateIndividualSignals=false;
}

void MasterFunctionGenerator::SetActiveFunctionGenerators(const std::vector<std::string>& names)
{
  DRIVER_LOG("",-1,-1);
  ownerOnly();
  if (names.size()==0)
  {
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "No Function Generators Selected" );
  }


  for (auto name : names)
  {
    if (std::any_of(allFunctionGenerators.begin(), allFunctionGenerators.end(),
          [name](std::shared_ptr<FunctionGeneratorImpl> fg){ return name==fg->GetName();}) == false)
    {
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Function Generator Not Found " + name);
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
  DRIVER_LOG("",-1,-1);
  clog << "MasterFG Timed out waiting" << std::endl;
  waitTimeout.disconnect();
  return false;
}

bool MasterFunctionGenerator::all_armed()
{
  DRIVER_LOG("",-1,-1);
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
  DRIVER_LOG("",-1,-1);
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
  DRIVER_LOG("",-1,-1);
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);

  std::shared_ptr<Slib::MainContext> context = Slib::MainContext::get_default();
  do
  {
    context->iteration(false);
    clock_gettime(CLOCK_MONOTONIC, &now);
    int dt_ms = (now.tv_sec - start.tv_sec)*1000 
              + (now.tv_nsec - start.tv_nsec)/1000000;
    if (dt_ms > timeout_ms) {
      throw saftbus::Error(saftbus::Error::INVALID_ARGS,"MasterFG: Timeout waiting for condition");
    }              
  } while (condition() == false) ;
}



}
