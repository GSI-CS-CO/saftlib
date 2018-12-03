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
	d-bus interface for WrMilGateway
	uses WrMilGatewayImpl
*/

 
#ifndef WR_MIL_GATEWAY_H_
#define WR_MIL_GATEWAY_H_

#include <deque>

#include "interfaces/WrMilGateway.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;

class WrMilGateway : public Owned, public iWrMilGateway
{
	
  public:
    typedef WrMilGateway_Service ServiceType;
    struct ConstructorType {
      Glib::ustring objectPath;
      TimingReceiver* dev;
    };
    
    static Glib::RefPtr<WrMilGateway> create(const ConstructorType& args);
    
    // iWrMilGateway overrides
    std::vector< guint32 > getRegisterContent() const;

    void StartSIS18();
    void StartESR();
    void ResetGateway();
    void KillGateway();


    guint32 getWrMilMagic() const;
    guint32 getFirmwareState() const;
    guint32 getEventSource() const;
    unsigned char getUtcTrigger() const;
    guint32 getEventLatency() const;
    guint32 getUtcUtcDelay() const;
    guint32 getTriggerUtcDelay() const;
    guint64 getUTCOffset() const;
    guint64 getNumMilEvents() const;
    guint32 getNumLateMilEvents() const;

    void setUtcTrigger(unsigned char val);
    void setEventLatency(guint32 val);
    void setUtcUtcDelay(guint32 val);
    void setTriggerUtcDelay(guint32 val);
    void setUTCOffset(guint64 val);
    
  protected:
    WrMilGateway(const ConstructorType& args);
    ~WrMilGateway();
    void Reset();
    void ownerQuit();
            
    TimingReceiver* dev;

    struct sdb_device wrmilgw_device; // store the LM32 device with WR-MIL-Gateway firmware running
    bool   have_wrmilgw;
    std::vector< guint32 > registerContent; 
};

}

#endif
