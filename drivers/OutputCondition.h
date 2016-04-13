/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#ifndef OUTPUT_CONDITION_H
#define OUTPUT_CONDITION_H

#include "interfaces/OutputCondition.h"
#include "Condition.h"

namespace saftlib {

class OutputCondition : public Condition, public iOutputCondition
{
  public:
    typedef OutputCondition_Service ServiceType;
    typedef Condition_ConstructorType ConstructorType;
    
    static Glib::RefPtr<OutputCondition> create(const ConstructorType& args);
    
    // iOutputCondition
    bool getOn() const;
    void setOn(bool val);
    
  protected:
    OutputCondition(const ConstructorType& args);
};

}

#endif
