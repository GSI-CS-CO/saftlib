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
#ifndef MASTER_FUNCTION_GENERATOR_H
#define MASTER_FUNCTION_GENERATOR_H

#include <deque>

#include "interfaces/MasterFunctionGenerator.h"
#include "FunctionGeneratorImpl.h"
#include "Owned.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>


namespace saftlib {

class TimingReceiver;

typedef boost::interprocess::allocator<ParameterTuple, boost::interprocess::managed_shared_memory::segment_manager>  ShmemAllocator;
typedef boost::interprocess::vector<ParameterTuple, ShmemAllocator> ParameterVector;


class MasterFunctionGenerator : public Owned, public iMasterFunctionGenerator
{
  public:
    typedef MasterFunctionGenerator_Service ServiceType;
    struct ConstructorType {
      std::string objectPath;
      TimingReceiver *tr;
      std::vector<std::shared_ptr<FunctionGeneratorImpl>> functionGenerators;            
    };
    
    static std::shared_ptr<MasterFunctionGenerator> create(const ConstructorType& args);
   
    // iMasterFunctionGenerator overrides
    void Arm();
    void Abort(bool);

    void InitializeSharedMemory(const std::string& shared_memory_name);

    void AppendParameterTuplesForBeamProcess(int beam_process, bool arm, bool wait_for_arm_ack);

	bool AppendParameterSets(const std::vector< std::vector< int16_t > >& coeff_a, const std::vector< std::vector< int16_t > >& coeff_b, const std::vector< std::vector< int32_t > >& coeff_c, const std::vector< std::vector< unsigned char > >& step, const std::vector< std::vector< unsigned char > >& freq, const std::vector< std::vector< unsigned char > >& shift_a, const std::vector< std::vector< unsigned char > >& shift_b, bool arm, bool wait_for_arm_ack);    
    std::vector<uint32_t> ReadExecutedParameterCounts();
    std::vector<uint64_t> ReadFillLevels();
    void Flush();
    void setStartTag(uint32_t val);
    uint32_t getStartTag() const;

    void setGenerateIndividualSignals(bool);
    bool getGenerateIndividualSignals() const;

    std::vector<std::string> ReadAllNames();
    std::vector<std::string> ReadNames();
    std::vector<int> ReadArmed();
    std::vector<int> ReadEnabled();
    std::vector<int> ReadRunning();
    void ResetActiveFunctionGenerators();
    void SetActiveFunctionGenerators(const std::vector<std::string>&);

  protected:
    MasterFunctionGenerator(const ConstructorType& args);
    ~MasterFunctionGenerator();
    
    void arm_all();
    void reset_all();

    void ownerQuit();
    
    void on_fg_running(std::shared_ptr<FunctionGeneratorImpl>& fg, bool);
    void on_fg_armed(std::shared_ptr<FunctionGeneratorImpl>& fg, bool);
    void on_fg_enabled(std::shared_ptr<FunctionGeneratorImpl>& fg, bool);
    void on_fg_started(std::shared_ptr<FunctionGeneratorImpl>& fg, uint64_t);
    void on_fg_stopped(std::shared_ptr<FunctionGeneratorImpl>& fg, uint64_t time, bool abort, bool hardwareUnderflow, bool microcontrollerUnderflow);
    void on_fg_refill(std::shared_ptr<FunctionGeneratorImpl>& fg);


    bool all_armed();
    bool all_stopped();
    bool WaitTimeout();
    void waitForCondition(std::function<bool()> condition, int timeout_ms);

    TimingReceiver *tr;
  	std::vector<std::shared_ptr<FunctionGeneratorImpl>> allFunctionGenerators;      
  	std::vector<std::shared_ptr<FunctionGeneratorImpl>> activeFunctionGenerators;      
    uint32_t startTag;
    bool generateIndividualSignals;
    sigc::connection waitTimeout; 

    std::map <int,std::vector<ParameterTuple>> parametersForBeamProcess;
    std::unique_ptr<boost::interprocess::managed_shared_memory> shm_params;
    boost::interprocess::interprocess_mutex* shm_mutex;
    std::map<std::string,ParameterVector*> paramVectors;
};

}

#endif
