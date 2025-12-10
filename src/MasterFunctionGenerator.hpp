/*  Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#ifndef MASTER_FUNCTION_GENERATOR_HPP_
#define MASTER_FUNCTION_GENERATOR_HPP_

#include <deque>

//#include "interfaces/MasterFunctionGenerator.h"
#include "Owned.hpp"
#include "FunctionGeneratorImpl.hpp"

// @saftbus-export
#include "Time.hpp"
// @saftbus-export
#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>


namespace saftlib {

class TimingReceiver;

typedef boost::interprocess::allocator<ParameterTuple, boost::interprocess::managed_shared_memory::segment_manager>  ShmemAllocator;
typedef boost::interprocess::vector<ParameterTuple, ShmemAllocator> ParameterVector;



/// @brief Interface to multiple Function Generators
///
/// Operation of function generators is aggregated to reduce
/// the number of d-bus operations required.
/// 
class MasterFunctionGenerator : public Owned 
{
  public:

    MasterFunctionGenerator(saftbus::Container *container, 
                            ///const std::string &fg_name, 
                            const std::string &object_path, 
                            std::vector<std::shared_ptr<FunctionGeneratorImpl> > functionGenerators);
    ~MasterFunctionGenerator();
    

    // typedef MasterFunctionGenerator_Service ServiceType;
    // struct ConstructorType {
    //   std::string objectPath;
    //   TimingReceiver *tr;
    //   std::vector<std::shared_ptr<FunctionGeneratorImpl>> functionGenerators;            
    // };
    
    // static std::shared_ptr<MasterFunctionGenerator> create(const ConstructorType& args);
   
    // iMasterFunctionGenerator overrides

    /// @brief Enable all function generators that have data and arm them.
    ///
    /// A function generator can only be Armed if FillLevel is non-zero. 
    /// An Enabled function generator can not be Armed again until it
    /// either completes waveform generation or the user calls Abort.
    /// Arming a function generator takes time.
    /// Wait for Armed to transition to true before sending StartTag.
    /// 
    // @saftbus-export
    void Arm();

    /// @brief Abort waveform generation in all function generators.
    ///
    /// This directs the hardware to stop waveform generation. If the
    /// function generator was Armed, it is disarmed and disabled, without
    /// outputting any waveform data. If the function generator is running, 
    /// output is Stopped at the current value and disabled. Aborting a
    /// function generator takes time, so even after a call to Abort,
    /// the function generator might still be Started. However, it will
    /// reach the disabled state as quickly as it can, transitioning
    /// through Stopped as usual. If the Owner of a FunctionGenerator 
    /// quits without running Disown, the Abort is run automatically.
    /// 
    // @saftbus-export
    void Abort();

    /// @brief Initialze a boost managed_shared_memory region
    ///
    /// It should contain:
    /// mutex
    /// ParameterVector
    /// 
    // @saftbus-export
    void InitializeSharedMemory(const std::string& shared_memory_name);

    // @saftbus-export
    void AppendParameterTuplesForBeamProcess(int beam_process);
    ///
    /// @brief For each function generator, append parameter tuples describing
    /// waveform to generate.
    /// Each parameter is sent as a vector of vectors: per FG then per tuple element
    /// Parameters are sent as a vector of vectors. 
    /// The outside vectors contain a coefficient vector for each FG and must be of the same size
    /// and less or equal the number of active FGs.
    /// The coefficient vectors for each FG's parameter set must be the same size
    /// but different FGs may have different parameter set sizes. 
    /// If there is no data for an individual function generator an empty vector 
    /// should be sent.
    /// 
    /// @coeff_a:  Quadratic coefficient (a), 16-bit signed
    /// @coeff_b:  Linear coefficient (b),    16-bit signed
    /// @coeff_c:  Constant coefficient (c),  32-bit signed
    /// @step:     Number of interpolated samples (0=250, 1=500, 2=1000, 3=2000, 4=4000, 5=8000, 6=16000, or 7=32000)
    /// @freq:     Output sample frequency (0=16kHz, 1=32kHz, 2=64kHz, 3=125kHz, 4=250kHz, 5=500kHz, 6=1MHz, or 7=2MHz)
    /// @shift_a:  Exponent of coeff_a, 6-bit unsigned; a = coeff_a*2^shift_a
    /// @shift_b:  Exponent of coeff_b, 6-bit unsigned; b = coeff_b*2^shift_b
    ///
    /// @low_fill: Fill level remains low for at least one FG - use ReadFillLevels
    /// 
    ///
    /// This function appends the parameter vectors (which must be equal in length)
    /// to the FIFO of remaining waveform to generate. Each parameter set (coefficients)
    /// describes a number of output samples in the generated wave form. Parameter
    /// sets are executed in order until no more remain.
    /// 
    /// If the fill level is not high enough, this method returns true. 
    /// Only once this function has returned false can you await the Refill signal.
    ///
    /// At each step, the function generator outputs high_bits(c*2^32 + b*t + c*t*t),
    /// where t ranges from 0 to numSteps-1. high_bits are the high OutputWindowSize
    /// bits of the resulting 64-bit signed value.
    /// 
    // @saftbus-export
  	bool AppendParameterSets(const std::vector< std::vector< int16_t > >& coeff_a, const std::vector< std::vector< int16_t > >& coeff_b, const std::vector< std::vector< int32_t > >& coeff_c, const std::vector< std::vector< unsigned char > >& step, const std::vector< std::vector< unsigned char > >& freq, const std::vector< std::vector< unsigned char > >& shift_a, const std::vector< std::vector< unsigned char > >& shift_b);    

    /// @brief Number of parameter tuples executed by each function generator
    /// 
    /// This counts the total number of parameter tuples executed since the last
    /// Started signal. Obviously, if the function generator is running, the returned
    /// value will be old.
    /// 
    /// @return              Number tuples executed by hardware.
    /// 
    // @saftbus-export
    std::vector<uint32_t> ReadExecutedParameterCounts();

    /// @brief Remaining waveform data in nanoseconds for each FG.
    /// 
    /// The SAFTd has sufficient parameters buffered to supply the function generator with 
    /// data for the specified time in nanoseconds. Note, due to the slow nature of software,
    /// if the function generator is currently running, the read value will already be 
    /// out-of-date upon return. This property should be used for informational use only.
    /// 
    /// @return    Remaining waveform data in nanoseconds for each FG.
    /// 
    // @saftbus-export
    std::vector<uint64_t> ReadFillLevels();

    /// @brief Empty the parameter tuple set of all function generators.
    ///
    /// Flush may only be called when not Enabled.
    /// Flush does not clear the ExecutedParameterCount.
    /// 
    // @saftbus-export
    void Flush();

    /// @brief The SCUbus tag which causes function generation to begin.
    /// 
    /// All function generators under control of the Master use the same tag.
    /// If the function generator is Armed and this tag is sent to the SCUbus, 
    /// then the function generator will begin generating the output waveform.
    /// StartTag may only be set when the FunctionGenerator is not Enabled.
    /// 
    // @saftbus-export
    void setStartTag(uint32_t val);
    // @saftbus-export
    uint32_t getStartTag() const;

    /// @brief If true, signals from the individual
    ///
    /// function generators are forwarded. The aggregate signals will still 
    /// be generated.
    /// This defaults to false to reduce the load on the d-bus message bus.
    /// 
    // @saftbus-export
    void setGenerateIndividualSignals(bool newvalue);
    // @saftbus-export
    bool getGenerateIndividualSignals() const;

    /// @brief Read the name for each available FG.
    ///
    /// @return Name of each available FG.
    /// 
    // @saftbus-export
    std::vector<std::string> ReadAllNames();

    /// @brief Read the name for each active FG.
    /// @return  Name of each active FG.
    /// 
    // @saftbus-export
    std::vector<std::string> ReadNames();

    /// @brief Read the armed state of each active FG.
    ///
    /// @return State of each FG.
    /// 
    // @saftbus-export
    std::vector<int> ReadArmed();

    /// @brief Read the enabled state of each active FG.
    /// @return  State of each FG.
    ///     
    // @saftbus-export
    std::vector<int> ReadEnabled();

    /// @brief Read the running state of each active FG.
    /// @return  State of each FG.
    ///     
    // @saftbus-export
    std::vector<int> ReadRunning();

    /// @brief Resets the list of active function generators handled by this master to all available FGs.
    /// 
    // @saftbus-export
    void ResetActiveFunctionGenerators();

    /// @brief Set the list of active function generators handled by this master to the names given.
    ///
    /// Format: fg-[SCUBusSlot]-[DeviceNumber]-[index]
    /// e.g. {"fg-3-0-0","fg-3-1-1"}
    /// 
    /// @param names: List of names identifying each FG to activate. 
    ///
    // @saftbus-export
    void SetActiveFunctionGenerators(const std::vector<std::string> &names);


    // Signals

    /// @brief Stopped signal forwarded from a single Function Generator
    ///
    /// @param name                     Name of the FG that generated the signal.
    /// @param time                     Time when function generation ended in nanoseconds since 1970
    /// @param abort                    stopped due to a call to Abort
    /// @param hardwareMacroUnderflow   A fatal error, indicating the SCUbus is congested
    /// @param microControllerUnderflow A fatal error, indicating the host CPU is overloaded
    ///
    /// The function generator stops either successfully (when all data has been sent), or 
    /// it stops due to an error. When an error occurs, the function generator stops and 
    /// holds its most recent value. This can occur due to two causes:
    ///
    /// hardwareMacroUnderflow, a fatal error indicating the hardware ran out of data.
    /// If the SCUbus is too busy, it can happen that the waveform data stored in the 
    /// function generator HDL is not refilled in time. This error can only be mitigated 
    /// by ensuring that the function generator does not share the SCUbus with other users.
    ///
    /// microControllerUnderflow, a fatal error indicating the microcontroller ran out of data.
    /// If the host CPU running this software is too busy, it can happen that the waveform
    /// data is not delivered to the microcontroller before the microcontroller runs out of data.
    /// This error can be mitigated by reducing the number of busy processes running on the system.
    ///
    /// Once the function generator has stopped, ExecutedParameterCount remains valid until the
    /// next time the function generator starts. After stopping, regardless of if the generation
    /// was successful or not, the parameter FIFO is cleared, Enabled is false, and this signal emitted.
    /// 
    // @saftbus-export
    sigc::signal< void , std::string  , saftlib::Time , bool , bool , bool > SigStopped;

    /// @brief Armed signal forwarded from a single Function Generator
    // @saftbus-export
    sigc::signal< void , std::string , bool  > Armed;

    /// @brief Enabled signal forwarded from a single Function Generator
    // @saftbus-export
    sigc::signal< void , std::string , bool  > Enabled;

    /// @brief Running signal forwarded from a single Function Generator
    // @saftbus-export
    sigc::signal< void , std::string , bool  > Running;

    /// @brief Started signal forwarded from a single Function Generator
    // @saftbus-export
    sigc::signal< void , std::string , saftlib::Time > SigStarted;

    /// @brief Refill signal forwarded from a single Function Generator
    // @saftbus-export
    sigc::signal< void , std::string > Refill;

    /// @brief All Function generators have stopped.
    ///
    /// This signal is generated when a function generator stops and all function generators
    /// controlled by this master have then stopped. 
    /// 
    /// @param time     Time when last function generation ended
    /// 
    // @saftbus-export
    sigc::signal< void , saftlib::Time > SigAllStopped;

    /// @brief   All Function generators that have a FillLevel>0 are armed.
    /// 
    /// This signal is generated when a function generator signals that it is armed, and
    /// all function generators controlled by this master that have FillLevel>0 are armed. 
    /// 
    // @saftbus-export
    sigc::signal< void > AllArmed;


    std::string getObjectPath();

  protected:
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

    //TimingReceiver *tr;
    std::string objectPath;
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
