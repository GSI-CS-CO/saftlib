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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <iostream>
#include <iomanip>
#include <map>

#include <assert.h>

#include "RegisteredObject.h"
#include "TimingReceiver.h"
#include "WrMilGateway.h"
#include "wr_mil_gw_regs.h" 
#include "clog.h"

namespace saftlib {

WrMilGateway::WrMilGateway(const ConstructorType& args)
 : Owned(args.objectPath),
   poll_period(200), // [ms]
   max_time_without_mil_events(10000), // 10 seconds
   receiver(args.receiver),
   base_addr(args.base_addr),
   have_wrmilgw(false)
{
  // get all LM32 devices and see if any of them has the WR-MIL-Gateway magic number
  std::vector<struct sdb_device> devices;
  receiver->getDevice().sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x54111351), devices);
  int cpu_idx = 0;
  for(auto lm32_ram_user: devices) {
    eb_data_t wr_mil_gw_magic_number = 0;
    receiver->getDevice().read(lm32_ram_user.sdb_component.addr_first + WR_MIL_GW_SHARED_OFFSET, 
                          EB_DATA32, 
                          &wr_mil_gw_magic_number);
    if (wr_mil_gw_magic_number == WR_MIL_GW_MAGIC_NUMBER) {
      wrmilgw_device = lm32_ram_user;
      base_addr = wrmilgw_device.sdb_component.addr_first;
      have_wrmilgw = true;
      break;
    }
    ++cpu_idx;
  }

  // just throw if there is no active firmware
  if (!have_wrmilgw) {
    throw IPC_METHOD::Error(IPC_METHOD::Error::FAILED, "No WR-MIL-Gateway found");
  }
  // the firmware is not running if the command value was not overwritten
  if (!firmwareRunning()) {
    throw IPC_METHOD::Error(IPC_METHOD::Error::FAILED, "WR-MIL-Gateway not running");
  }

  // reset all other LM32 CPUs to make sure that no other firmware disturbs our actions
  // (in particular the function generator firmware)
  receiver->getDevice().sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x3a362063), devices);
  for (auto reset_device: devices) {
    // use the first reset device
    guint32 set_bits = ~(1<<cpu_idx) & 0xff;
    guint32 clr_bits =  (1<<cpu_idx);
    // reset register offsets
    //0x4 -> GET 
    //0x8 -> SET 
    //0xc -> CLR
    const int SET = 0x8;
    const int CLR = 0xc;
    receiver->getDevice().write(reset_device.sdb_component.addr_first + CLR, 
                          EB_DATA32, 
                          clr_bits);
    receiver->getDevice().write(reset_device.sdb_component.addr_first + SET, 
                          EB_DATA32, 
                          set_bits);
    break;
  }

  firmware_running = firmwareRunning();
  firmware_state   = readRegisterContent(WR_MIL_GW_REG_STATE);
  event_source     = readRegisterContent(WR_MIL_GW_REG_EVENT_SOURCE);
  num_late_events  = readRegisterContent(WR_MIL_GW_REG_LATE_EVENTS);
  
  // poll every 1s some registers
  pollConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &WrMilGateway::poll), poll_period);
}

WrMilGateway::~WrMilGateway()
{
  pollConnection.disconnect();
}

bool WrMilGateway::firmwareRunning() const
{
  // intentionally cast away the constness, because this is a temporary modification of a register
  // and saft daemon makes sure that this method is not called by two instances simultaneously
  WrMilGateway *nonconst = const_cast<WrMilGateway*>(this);

   // see if the firmware is running (it should reset the CMD register to 0 after a command is put there)
   // submit a test command 
  nonconst->writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_TEST);
  usleep(10000);
  // command register will be cleared if the firmware is running;
  return (nonconst->readRegisterContent(WR_MIL_GW_REG_COMMAND) == 0);
}

bool WrMilGateway::getFirmwareRunning()  const
{
  bool new_firmware_running = firmwareRunning();

  // emit signal on change
  if (new_firmware_running != firmware_running) {
    firmware_running = new_firmware_running;
    SigFirmwareRunning(firmware_running);
  }

  return firmware_running;
}

guint32 WrMilGateway::readRegisterContent(guint32 reg_offset) const
{
    eb_data_t value;
    receiver->getDevice().read(base_addr + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, &value);
    return value;
}
void WrMilGateway::writeRegisterContent(guint32 reg_offset, guint32 value)
{
    receiver->getDevice().write(base_addr + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, (eb_data_t)value);
}

Glib::RefPtr<WrMilGateway> WrMilGateway::create(const ConstructorType& args)
{
  return RegisteredObject<WrMilGateway>::create(args.objectPath, args);
}

std::vector< guint32 > WrMilGateway::getRegisterContent() const
{
  std::vector<guint32> registerContent((WR_MIL_GW_REG_LAST-WR_MIL_GW_REG_MAGIC_NUMBER) / 4, 0);
  for (unsigned i = 0; i < registerContent.size(); ++i) {
    eb_data_t value;
    receiver->getDevice().read(base_addr + WR_MIL_GW_SHARED_OFFSET + 4*i, EB_DATA32, &value);
    registerContent.push_back(value);
  }
  return registerContent;
}

bool WrMilGateway::getInUse() const
{
  return time_without_mil_events <= max_time_without_mil_events;
}



bool WrMilGateway::poll()
{
  getFirmwareRunning();
  getFirmwareState();
  getEventSource();
  getNumLateMilEvents();

  // check if the gateway is used (translates events)
  guint64 new_num_mil_events = getNumMilEvents();
  if (num_mil_events != new_num_mil_events) {
    if (!getInUse()) {
      // in this case we change back to being "in use"
      SigInUse(true);
    }
    time_without_mil_events = 0;
    num_mil_events = new_num_mil_events;
  } else {
    bool inUse = getInUse();
    time_without_mil_events += poll_period;
    bool new_inUse = getInUse();
    if (inUse && !new_inUse) {
      // Not seen a MIL event for too long... 
      //  ... that counts as not being used because
      //  we expect event 255 (EVT_COMMAND) 
      //  every second in normal operation
      // Only produce this signal once
      SigInUse(false);
    }
  }

  return true; // return true to continue polling
}

void WrMilGateway::StartSIS18()
{
  // configure WR-MIL Gateway firmware to start operation as SIS18 Pulszentrale
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_SIS);
}
void WrMilGateway::StartESR()
{
  // configure WR-MIL Gateway firmware to start operation as ESR Pulszentrale
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_ESR);
}
void WrMilGateway::ResetGateway()
{
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_RESET);
}
void WrMilGateway::KillGateway()
{
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_KILL);
}


guint32 WrMilGateway::getWrMilMagic() const
{
  return readRegisterContent(WR_MIL_GW_REG_MAGIC_NUMBER);
}
guint32 WrMilGateway::getFirmwareState() const
{

  auto new_firmware_state = readRegisterContent(WR_MIL_GW_REG_STATE);
  if (firmware_state != new_firmware_state) {
    firmware_state = new_firmware_state;
    SigFirmwareState(firmware_state);
  }
  return firmware_state;
}
guint32 WrMilGateway::getEventSource() const
{
  auto new_event_source = readRegisterContent(WR_MIL_GW_REG_EVENT_SOURCE);
  if (event_source != new_event_source) {
    event_source = new_event_source;
    SigEventSource(event_source);
  }
  return event_source;
}
unsigned char WrMilGateway::getUtcTrigger() const
{
  return readRegisterContent(WR_MIL_GW_REG_UTC_TRIGGER);
}
guint32 WrMilGateway::getEventLatency() const
{
  return readRegisterContent(WR_MIL_GW_REG_LATENCY);
}
guint32 WrMilGateway::getUtcUtcDelay() const
{
  return readRegisterContent(WR_MIL_GW_REG_UTC_DELAY);
}
guint32 WrMilGateway::getTriggerUtcDelay() const
{
  return readRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY);
}
guint64 WrMilGateway::getUtcOffset() const
{
  guint64 result = readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO);
  return result;
}
guint64 WrMilGateway::getNumMilEvents() const
{
  guint64 result = readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_LO);
  return result;
}
guint32 WrMilGateway::getNumLateMilEvents() const
{
  guint32 new_num_late_events = readRegisterContent(WR_MIL_GW_REG_LATE_EVENTS);
  if (num_late_events != new_num_late_events) {
    // send the current number and the ones since last signal
    guint32 difference = 0;
    if (new_num_late_events >= num_late_events) {
      difference = new_num_late_events - num_late_events;
    }
    SigNumLateMilEvents(new_num_late_events, difference);
    num_late_events = new_num_late_events;
  }
  return num_late_events;
}

void WrMilGateway::setUtcTrigger(unsigned char val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_TRIGGER, val);
}
void WrMilGateway::setEventLatency(guint32 val)
{
  writeRegisterContent(WR_MIL_GW_REG_LATENCY, val);
}
void WrMilGateway::setUtcUtcDelay(guint32 val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_DELAY, val);
}
void WrMilGateway::setTriggerUtcDelay(guint32 val)
{
  writeRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY, val);
}
void WrMilGateway::setUtcOffset(guint64 val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO, val & 0x00000000ffffffff);
  val >>= 32;
  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO, val & 0x00000000ffffffff);
}



void WrMilGateway::Reset() 
{
}

void WrMilGateway::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  Reset();
}

}
