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
      std::string                objectPath;
      Device &                   device;
    };
    
    static std::shared_ptr<WrMilGateway> create(const ConstructorType& args);
    // iWrMilGateway overrides
    
    void StartSIS18();
    void StartESR();
    void ClearStatistics();
    void ResetGateway();
    void KillGateway();
    void UpdateOLED();
    void RequestFillEvent();

    std::vector< uint32_t > getRegisterContent()  const;
    std::vector< uint32_t > getMilHistogram()     const;
    uint32_t                getWrMilMagic()       const;
    uint32_t                getFirmwareState()    const;
    uint32_t                getEventSource()      const;
    unsigned char          getUtcTrigger()       const;
    uint32_t                getEventLatency()     const;
    uint32_t                getUtcUtcDelay()      const;
    uint32_t                getTriggerUtcDelay()  const;
    uint64_t                getUtcOffset()        const;
    uint64_t                getNumMilEvents()     const;
    std::vector< uint32_t > getLateHistogram()    const;
    uint32_t                getNumLateMilEvents() const;
    bool                   getFirmwareRunning()  const;
    bool                   getInUse()            const;

    void setUtcTrigger(unsigned char val);
    void setEventLatency(uint32_t val);
    void setUtcUtcDelay(uint32_t val);
    void setTriggerUtcDelay(uint32_t val);
    void setUtcOffset(uint64_t val);
    void setOpReady(bool val);
    
  protected:
    WrMilGateway(const ConstructorType& args);
    ~WrMilGateway();
    void Reset();
    void ownerQuit();

    void oledUpdate();

    // Polling method
    bool poll();
    const int poll_period; // [ms]


    uint32_t readRegisterContent(uint32_t reg_offset) const;
    void    writeRegisterContent(uint32_t reg_offset, uint32_t value);
    bool    firmwareRunning() const;

    // void irq_handler(eb_data_t msg) const;


    mutable bool    firmware_running;
    mutable uint32_t firmware_state;
    mutable uint32_t event_source;
    mutable uint32_t num_late_events;
    uint64_t num_mil_events;
    const uint32_t max_time_without_mil_events; // if time_without_events exceeds this, we conclude the gateway isn't used
    uint32_t time_without_mil_events;

    sigc::connection pollConnection;

    eb_address_t  oled_reset;
    eb_address_t  oled_char;
    eb_address_t  mil_events_present;
    eb_address_t  mil_event_read_and_pop;


    //std::shared_ptr<TimingReceiver> receiver;
    Device &                  device;

    struct sdb_device         wrmilgw_device; // store the LM32 device with WR-MIL-Gateway firmware running
    eb_address_t              base_addr;
    etherbone::sdb_msi_device sdb_msi_base;
    sdb_device                mailbox;
    eb_address_t              irq;
    bool                      have_wrmilgw;
    bool                      idle;

    eb_address_t              mailbox_slot_address;
    unsigned mbx_slot;

};

}

#endif
