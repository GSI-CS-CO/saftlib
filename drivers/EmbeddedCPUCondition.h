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
#ifndef EMBEDDED_CPU_CONDITION_H
#define EMBEDDED_CPU_CONDITION_H

#include "interfaces/EmbeddedCPUCondition.h"
#include "Condition.h"

namespace saftlib {

class EmbeddedCPUCondition : public Condition, public iEmbeddedCPUCondition
{
  public:
    typedef EmbeddedCPUCondition_Service ServiceType;
    typedef Condition_ConstructorType ConstructorType;
    
    static std::shared_ptr<EmbeddedCPUCondition> create(const ConstructorType& args);
    
    // iEmbeddedCPUCondition
    guint32 getTag() const;
    void setTag(guint32 val);
    
  protected:
    EmbeddedCPUCondition(const ConstructorType& args);
};

}

#endif
