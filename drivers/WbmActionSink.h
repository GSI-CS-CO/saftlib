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
#ifndef WBM_ACTION_SINK_H
#define WBM_ACTION_SINK_H

#include "interfaces/WbmActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class WbmActionSink : public ActionSink, public iWbmActionSink
{
  public:
    typedef WbmActionSink_Service ServiceType;
    struct ConstructorType {
      std::string objectPath;
      TimingReceiver* dev;
      std::string name;
      unsigned channel;
      eb_address_t acwbm;
    };
    
    static std::shared_ptr<WbmActionSink> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // iWbmAcitonSink
    std::string NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag);
    //void InjectTag(uint32_t tag);

    void ExecuteMacro(uint32_t idx);
    void RecordMacro(uint32_t idx, const std::vector< std::vector< uint32_t > >& commands);
    void ClearMacro(uint32_t idx);
    void ClearAllMacros();
    // Property getters
    unsigned char getStatus() const;
    uint32_t getMaxMacros() const;
    uint32_t getMaxSpace() const;
    bool getEnable() const;
    uint32_t getLastExecutedIdx() const;
    uint32_t getLastRecordedIdx() const;
    // Property setters
    void setEnable(bool val);

  protected:
    WbmActionSink(const ConstructorType& args);
    eb_address_t acwbm;
};

}

#endif
