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
#ifndef SOFTWARE_CONDITION_H
#define SOFTWARE_CONDITION_H

#include "interfaces/SoftwareCondition.h"
#include "Condition.h"

namespace saftlib {

class SoftwareCondition : public Condition, public iSoftwareCondition
{
  public:
    typedef SoftwareCondition_Service ServiceType;
    typedef Condition_ConstructorType ConstructorType;
    
    static std::shared_ptr<SoftwareCondition> create(const ConstructorType& args);
    
    // iSoftwareCondition
    // -> Action
    
  protected:
    SoftwareCondition(const ConstructorType& args);
};

}

#endif
