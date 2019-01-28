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
#ifndef EMBEDDED_CPU_ACTION_SINK_H
#define EMBEDDED_CPU_ACTION_SINK_H

#include "interfaces/EmbeddedCPUActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class EmbeddedCPUActionSink : public ActionSink, public iEmbeddedCPUActionSink
{
  public:
    typedef EmbeddedCPUActionSink_Service ServiceType;
    struct ConstructorType {
      std::string objectPath;
      TimingReceiver* dev;
      std::string name;
      unsigned channel;
      eb_address_t embedded_cpu;
    };
    
    static std::shared_ptr<EmbeddedCPUActionSink> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // iEmbeddedCPUAcitonSink
    std::string NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 tag);
    
  protected:
    EmbeddedCPUActionSink(const ConstructorType& args);
    eb_address_t embedded_cpu;
};

}

#endif
