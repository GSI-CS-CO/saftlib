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
#ifndef SCUBUS_ACTION_SINK_H
#define SCUBUS_ACTION_SINK_H

#include "interfaces/SCUbusActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class SCUbusActionSink : public ActionSink, public iSCUbusActionSink
{
  public:
    typedef SCUbusActionSink_Service ServiceType;
    struct ConstructorType {
      std::string objectPath;
      TimingReceiver* dev;
      std::string name;
      unsigned channel;
      eb_address_t scubus;
    };
    
    static std::shared_ptr<SCUbusActionSink> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // iSCUbusAcitonSink
    std::string NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag);
    void InjectTag(uint32_t tag);
    
  protected:
    SCUbusActionSink(const ConstructorType& args);
    eb_address_t scubus;
};

}

#endif
