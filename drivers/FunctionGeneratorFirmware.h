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

 
#ifndef FUNCTION_GENERATOR_FIRMWARE_H
#define FUNCTION_GENERATOR_FIRMWARE_H

#include <deque>

#include "interfaces/FunctionGeneratorFirmware.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;

class FunctionGeneratorFirmware : public Owned, public iFunctionGeneratorFirmware
{
	
  public:
    typedef FunctionGeneratorFirmware_Service ServiceType;
    struct ConstructorType {
      std::string objectPath;
      std::shared_ptr<TimingReceiver> receiver;
    };
    
    static std::shared_ptr<FunctionGeneratorFirmware> create(const ConstructorType& args);
    
    // iFunctionGenerator overrides
    std::map<std::string, std::string> Scan();
    
  protected:
    FunctionGeneratorFirmware(const ConstructorType& args);
    ~FunctionGeneratorFirmware();

    std::shared_ptr<TimingReceiver> receiver;
 
};

}

#endif
