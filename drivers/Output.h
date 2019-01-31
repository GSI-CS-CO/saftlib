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
#ifndef OUTPUT_H
#define OUTPUT_H

#include "InoutImpl.h"
#include "ActionSink.h"
#include "interfaces/Output.h"
#include "interfaces/iOutputActionSink.h"

namespace saftlib {

class Output : public ActionSink, public iOutputActionSink
{
  public:
    typedef Output_Service ServiceType;
    struct ConstructorType {
      std::string name;
      std::string objectPath;
      std::string partnerPath;
      TimingReceiver* dev;
      unsigned channel;
      unsigned num;
      std::shared_ptr<InoutImpl> impl;
      sigc::slot<void> destroy;
    };

    static std::shared_ptr<Output> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // Methods
    std::string NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, bool on);
    void WriteOutput(bool value);
    bool ReadOutput();
    bool StartClock(double high_phase, double low_phase, uint64_t phase_offset);
    bool StopClock();
    
    // Property getters
    bool getOutputEnable() const;
    bool getSpecialPurposeOut() const;
    bool getBuTiSMultiplexer() const;
    bool getPPSMultiplexer() const;
    bool getOutputEnableAvailable() const;
    bool getSpecialPurposeOutAvailable() const;
    std::string getLogicLevelOut() const;
    std::string getTypeOut() const;
    std::string getInput() const;
    
    // Property setters
    void setOutputEnable(bool val);
    void setSpecialPurposeOut(bool val);
    void setBuTiSMultiplexer(bool val);
    void setPPSMultiplexer(bool val);
    
    // Property signals
    //   sigc::signal< void, bool > OutputEnable;
    //   sigc::signal< void, bool > SpecialPurposeOut;
    //   sigc::signal< void, bool > BuTiSMultiplexer;
  
  protected:
    Output(const ConstructorType& args);
    std::shared_ptr<InoutImpl> impl;
    std::string partnerPath;
};

}

#endif /* OUTPUT_H */
