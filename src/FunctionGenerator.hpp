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
 
 
 
/*
	d-bus interface for FunctionGenerator
	uses FunctionGeneratorImpl
*/

 
#ifndef FUNCTION_GENERATOR_HPP_
#define FUNCTION_GENERATOR_HPP_

#include "Owned.hpp"
// @saftbus-export
#include "Time.hpp"

#include <deque>
#include "FunctionGeneratorImpl.hpp"

namespace saftlib {

// class TimingReceiver;


/// @brief Function Generator for creating timing triggered waveforms
/// 
/// The function generator creates a waveform from a piecewise continuous sequence of
/// second order polynomials. These polynomials are specified using a fixed-width 
/// "parameter tuple" format. Once triggered, the function generator outputs samples
/// created from each tupled polynomial until it exhausts the specified piecewise
/// width of the polynomial, whereupon it begins outputting samples from the next
/// tupled polynomial.
/// 
/// The general work-flow to use the FunctionGenerator is to append sets of tuples
/// describing the waveform using the AppendParameterSet function. Then, select a
/// SCUbus timing tag (StartTag) whose appearance should trigger waveform generation.
/// Finally, the FunctionGenerator is armed by calling Arm, whereafter it may be
/// triggered manually via the SCUbusActionSink->InjectTag method, or by a timing
/// event sent by the data master, which is configured to create the StartTag.
/// 
/// If you need an unending waveform, follow the steps above, but additionally monitor 
/// the Refill signal, and append additional parameter tuples until low_fill is false.
/// 
/// The sequence of signals of successful waveform generation occur in this order: 
///   Enabled(true)
///   Armed(true)
///   Armed(false)
///   Started
///   Running(true)
///   Refill
///   Running(false)
///   Stopped
///   Enabled(false)
/// 
class FunctionGenerator : public Owned 
{
	
  public:

    // FunctionGenerator(const ConstructorType& args);
    FunctionGenerator(saftbus::Container *container, const std::string &fg_name, const std::string &object_path, std::shared_ptr<FunctionGeneratorImpl> impl);
    ~FunctionGenerator();

    // typedef FunctionGenerator_Service ServiceType;
    // struct ConstructorType {
    //     std::string objectPath;
    //     TimingReceiver *dev;
    //     std::shared_ptr<FunctionGeneratorImpl> functionGeneratorImpl;            
    // };
    
    // static std::shared_ptr<FunctionGenerator> create(saftbus::Container *container, const std::string &object_path, TimingReceiver *timing_receiver, std::shared_ptr<FunctionGeneratorImpl> impl);
    

    /// @brief Enable the function generator and arm it.
    ///
    /// A function generator can only be Armed if FillLevel is non-zero. 
    /// An Enabled function generator can not be Armed again until it
    /// either completes waveform generation or the user calls Abort.
    /// Arming a function generator takes time.
    /// Wait for Armed to transition to true before sending StartTag.
    /// 
    ///
    // @saftbus-export
    void Arm();

    /// @brief Abort waveform generation.
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

    /// @brief   Remaining waveform data in nanoseconds.
    ///
    /// The SAFTd has sufficient parameters buffered to supply the function generator with 
    /// data for the specified time in nanoseconds. Note, due to the slow nature of software,
    /// if the function generator is currently running, the read value will already be 
    /// out-of-date upon return. This property should be used for informational use only.
    /// @return  Remaining waveform data in nanoseconds.
    ///
    // @saftbus-export
    uint64_t ReadFillLevel();

    /// @brief          Append parameter tuples describing waveform to generate.
    ///    
    /// @param coeff_a  Quadratic coefficient (a), 16-bit signed
    /// @param coeff_b  Linear coefficient (b),    16-bit signed
    /// @param coeff_c  Constant coefficient (c),  32-bit signed
    /// @param step     Number of interpolated samples (0=250, 1=500, 2=1000, 3=2000, 4=4000, 5=8000, 6=16000, or 7=32000)
    /// @param freq     Output sample frequency (0=16kHz, 1=32kHz, 2=64kHz, 3=125kHz, 4=250kHz, 5=500kHz, 6=1MHz, or 7=2MHz)
    /// @param shift_a  Exponent of coeff_a, 6-bit unsigned; a = coeff_a*2^shift_a
    /// @param shift_b  Exponent of coeff_b, 6-bit unsigned; b = coeff_b*2^shift_b
    /// @param low_fill Fill level remains too low
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
    bool AppendParameterSet(const std::vector< int16_t >& coeff_a, const std::vector< int16_t >& coeff_b, const std::vector< int32_t >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b);

    /// @brief Empty the parameter tuple set.
    ///
    /// Flush may only be called when not Enabled.
    /// Flush does not clear the ExecutedParameterCount.
    /// 
    // @saftbus-export
    void Flush();

    /// @brief Version of the hardware macro
    /// @return Version of the hardware macro
    // @saftbus-export
    uint32_t getVersion() const;

    /// @brief Slot number of the slave card
    /// @return Slot number of the slave card
    // @saftbus-export
    unsigned char getSCUbusSlot() const;

    /// @brief number of the hardware macro inside of the slave card
    /// @return number of the hardware macro inside of the slave card
    // @saftbus-export
    unsigned char getDeviceNumber() const;

    /// @brief Number of bits output by the function generator
    /// @return Number of bits output by the function generator
    // @saftbus-export
    unsigned char getOutputWindowSize() const;

    /// @brief Hardware is allowed to generate an output waveform.
    /// If a function generator is disabled, its output is constant.
    /// Enabled is set to true using Arm and transitions to false
    /// either upon completion or after a call to Abort.
    /// SigEnabled is emitted when this changes.
    /// @return true if enabled
    ///
    // @saftbus-export
    bool getEnabled() const;

    /// @brief Upon receipt of StartTag, output will begin.
    ///
    /// If a function generator is Armed, it is also Enabled. Once waveform
    /// data has been loaded into the function generator, Arm it. Shortly
    /// thereafter (once the hardware is ready), the Armed property will
    /// transition to true, indicating the hardware is ready to react. Once
    /// StartTag is received, Armed changes to false and Started is emitted.
    /// SigArmed is emitted when this changes.
    /// @return true if armed
    ///
    // @saftbus-export
    bool getArmed() const;

    /// @brief The function generator is currently producing a waveform.
    ///
    /// Function generation is started by sending a SCUbus tag which matches the
    /// StartTag property of an Armed function generator. When this property 
    /// transitions to true, the Started signal is emitted. When this property
    /// transitions to false, the Stopped signal is emitted.
    /// SigRunning or  is emitted when this changes.
    /// @return true if running
    ///
    // @saftbus-export
    bool getRunning() const;


    /// @brief The SCUbus tag which causes function generation to begin.
    ///
    /// If the function generator is Armed and this tag is sent to the SCUbus, 
    /// then the function generator will begin generating the output waveform.
    /// StartTag may only be set when the FunctionGenerator is not Enabled.
    /// @return SCUbus tag
    ///
    // @saftbus-export
    uint32_t getStartTag() const;
    // @saftbus-export
    void setStartTag(uint32_t val);


    /// @brief   Number of parameter tuples executed by hardware.
    ///
    /// This counts the total number of parameter tuples executed since the last
    /// Started signal. Obviously, if the function generator is running, the returned
    /// value will be old.
    /// @return  Number tuples executed by hardware.
    ///
    // @saftbus-export
    uint32_t ReadExecutedParameterCount();



    // Signals
    /// @brief Hardware is allowed to generate an output waveform.
    /// If a function generator is disabled, its output is constant.
    /// Enabled is set to true using Arm and transitions to false
    /// either upon completion or after a call to Abort.
    /// The current state of Enabled can be obtained using getEnabled().
    ///
    // @saftbus-export
    sigc::signal< void , bool > SigEnabled;

    /// @brief Upon receipt of StartTag, output will begin.
    /// If a function generator is Armed, it is also Enabled. Once waveform
    /// data has been loaded into the function generator, Arm it. Shortly
    /// thereafter (once the hardware is ready), the Armed property will
    /// transition to true, indicating the hardware is ready to react. Once
    /// StartTag is received, Armed changes to false and Started is emitted.
    /// getArmed() method can be used to obtain the current state.
    ///
    // @saftbus-export
    sigc::signal< void , bool > SigArmed;

    /// @brief The function generator is currently producing a waveform.
    ///
    /// Function generation is started by sending a SCUbus tag which matches the
    /// StartTag property of an Armed function generator. When this property 
    /// transitions to true, the Started signal is emitted. When this property
    /// transitions to false, the Stopped signal is emitted.
    /// getRunning() method can be used to obtain the current state.
    ///
    // @saftbus-export
    sigc::signal< void , bool > SigRunning;

    /// @brief Function generation has begun.
    ///
    /// This signal notifies software when function generation has begun,
    /// possibly to update a GUI or other user-facing status information.
    /// @param time Time when function generation began in nanoseconds since 1970
    ///
    // @saftbus-export
    sigc::signal< void , saftlib::Time > SigStarted;

    /// @brief                          Function generation has ended.
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
    sigc::signal< void , saftlib::Time , bool , bool , bool > SigStopped;

    /// @brief More tuples must be appended to ensure uninterrupted waveform.
    ///
    /// In order to guarantee an uninterrupted supply of data to the
    /// function generator, there should be data buffered in the SAFTd. 
    /// When the buffer fill level gets too low, this signal is emitted,
    /// and you should run AppendParameterSet.  If you do not, function
    /// generation will cease, signalling successful completion of the
    /// waveform with Stopped.
    /// 
    // @saftbus-export
    sigc::signal< void > Refill;


    std::string getObjectPath();

  protected:
    void Reset();
    void ownerQuit();
            
    TimingReceiver *dev;
    
    void on_fg_running(bool);
    void on_fg_armed(bool);
    void on_fg_enabled(bool);
    void on_fg_refill();
    void on_fg_started(uint64_t);
    void on_fg_stopped(uint64_t, bool, bool, bool);


    std::string name;
    std::string objectPath;
   	std::shared_ptr<FunctionGeneratorImpl> fgImpl;      
    
};

}

#endif
