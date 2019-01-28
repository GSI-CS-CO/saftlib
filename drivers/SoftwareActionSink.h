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
#ifndef SOFTWARE_ACTION_SINK_H
#define SOFTWARE_ACTION_SINK_H

#include "interfaces/SoftwareActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class SoftwareActionSink : public ActionSink, public iSoftwareActionSink
{
  public:
    typedef SoftwareActionSink_Service ServiceType;
    struct ConstructorType {
      std::string objectPath;
      TimingReceiver* dev;
      std::string name;
      unsigned channel;
      unsigned num;
      eb_address_t queue;
      sigc::slot<void> destroy;
    };
    
    static std::shared_ptr<SoftwareActionSink> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // override receiveMSI to also pop the software queue
    void receiveMSI(guint8 code);
    
    // iSoftwareAcitonSink
    std::string NewCondition(bool active, guint64 id, guint64 mask, gint64 offset);
    
  protected:
    SoftwareActionSink(const ConstructorType& args);
    eb_address_t queue;
};

}

#endif
