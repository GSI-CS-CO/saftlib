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
#ifndef INPUT_H
#define INPUT_H

#include "InoutImpl.h"
#include "EventSource.h"
#include "interfaces/Input.h"
#include "interfaces/iInputEventSource.h"

namespace saftlib {

class Input : public EventSource, public iInputEventSource
{
  public:
    typedef Input_Service ServiceType;
    struct ConstructorType {
      std::string name;
      std::string objectPath;
      std::string partnerPath;
      TimingReceiver* dev;
      eb_address_t tlu;
      unsigned channel;
      std::shared_ptr<InoutImpl> impl;
      sigc::slot<void> destroy;
    };

    static std::shared_ptr<Input> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // Methods
    bool ReadInput();
    // Property getters
    uint32_t getStableTime() const ;
    bool getInputTermination() const;
    bool getSpecialPurposeIn() const;
    bool getInputTerminationAvailable() const;
    bool getSpecialPurposeInAvailable() const;
    std::string getLogicLevelIn() const;
    std::string getTypeIn() const;
    std::string getOutput() const;
    // Property setters
    void setStableTime(uint32_t val);
    void setInputTermination(bool val);
    void setSpecialPurposeIn(bool val);
    
    // Property signals
    //  sigc::signal< void, uint32_t > StableTime;
    //  sigc::signal< void, bool > InputTermination;
    //  sigc::signal< void, bool > SpecialPurposeIn;
    
    // From iEventSource
    uint64_t getResolution() const;
    uint32_t getEventBits() const;
    bool getEventEnable() const;
    uint64_t getEventPrefix() const;
    
    void setEventEnable(bool val);
    void setEventPrefix(uint64_t val);
    //  sigc::signal< void, bool > EventEnable;
    //  sigc::signal< void, uint64_t > EventPrefix;
    
  protected:
    Input(const ConstructorType& args);
    std::shared_ptr<InoutImpl> impl;
    std::string partnerPath;
    eb_address_t tlu;
    unsigned channel;
    bool enable;
    uint64_t event;
    uint32_t stable;
    
    void configInput();
};

}

#endif /* INPUT_H */
