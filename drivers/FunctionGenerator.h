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

 
#ifndef FUNCTION_GENERATOR_H
#define FUNCTION_GENERATOR_H

#include <deque>


#include "interfaces/FunctionGenerator.h"
#include "FunctionGeneratorImpl.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;

class FunctionGenerator : public Owned, public iFunctionGenerator
{
	
  public:
    typedef FunctionGenerator_Service ServiceType;
    struct ConstructorType {
        std::string objectPath;
        TimingReceiver *dev;
        std::shared_ptr<FunctionGeneratorImpl> functionGeneratorImpl;            
    };
    
    static std::shared_ptr<FunctionGenerator> create(const ConstructorType& args);
    
    // iFunctionGenerator overrides
    // saftbus method
    void Arm();
    // saftbus method
    void Abort();
    // saftbus method
    uint64_t ReadFillLevel();
    // saftbus method
    bool AppendParameterSet(const std::vector< int16_t >& coeff_a, const std::vector< int16_t >& coeff_b, const std::vector< int32_t >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b);
    // saftbus method
    void Flush();
    // saftbus property
    uint32_t getVersion() const;
    // saftbus property
    unsigned char getSCUbusSlot() const;
    // saftbus property
    unsigned char getDeviceNumber() const;
    // saftbus property
    unsigned char getOutputWindowSize() const;
    // saftbus property
    bool getEnabled() const;
    // saftbus property
    bool getArmed() const;
    // saftbus property
    bool getRunning() const;
    // saftbus property
    uint32_t getStartTag() const;
    // saftbus method
    uint32_t ReadExecutedParameterCount();
    // saftbus property
    void setStartTag(uint32_t val);
    
  protected:
    FunctionGenerator(const ConstructorType& args);
    ~FunctionGenerator();
    void Reset();
    void ownerQuit();
            
    TimingReceiver *dev;
    
    void on_fg_running(bool);
    void on_fg_armed(bool);
    void on_fg_enabled(bool);
    void on_fg_refill();
    void on_fg_started(uint64_t);
    void on_fg_stopped(uint64_t, bool, bool, bool);

   	std::shared_ptr<FunctionGeneratorImpl> fgImpl;      
    
};

}

#endif
