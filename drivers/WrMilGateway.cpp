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

#include <unistd.h>

#include "RegisteredObject.h"
#include "TimingReceiver.h"
#include "WrMilGateway.h"
#include "wr_mil_gw_regs.h" 
#include "clog.h"

#define LM32_RAM_USER_VENDOR      0x651       //vendor ID
#define LM32_RAM_USER_PRODUCT     0x54111351  //product ID
#define LM32_RAM_USER_VMAJOR      1           //major revision
#define LM32_RAM_USER_VMINOR      1           //minor revision

#define LM32_CLUSTER_ROM_VENDOR   0x651
#define LM32_CLUSTER_ROM_PRODUCT  0x10040086

#define MSI_MAILBOX_VENDOR        0x651
#define MSI_MAILBOX_PRODUCT       0xfab0bdd8


namespace saftlib {

WrMilGateway::WrMilGateway(const ConstructorType& args)
 : Owned(args.objectPath),
   poll_period(1000), // [ms]
   max_time_without_mil_events(10000), // 10 seconds
   time_without_mil_events(max_time_without_mil_events),
   device(args.device),
   sdb_msi_base(args.sdb_msi_base),
   mailbox(args.mailbox),
   irq(args.device.request_irq(args.sdb_msi_base, sigc::mem_fun(*this, &WrMilGateway::irq_handler))),
   have_wrmilgw(false)
{


  // get all LM32 devices and see if any of them has the WR-MIL-Gateway magic number
  std::vector<struct sdb_device> devices;
  device.sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x54111351), devices);
  int cpu_idx = 0;
  for(auto lm32_ram_user: devices) {
    eb_data_t wr_mil_gw_magic_number = 0;
    device.read(lm32_ram_user.sdb_component.addr_first + WR_MIL_GW_SHARED_OFFSET, 
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
    throw saftbus::Error(saftbus::Error::FAILED, "No WR-MIL-Gateway found");
  }
  // the firmware is not running if the command value was not overwritten
  if (!firmwareRunning()) {
    throw saftbus::Error(saftbus::Error::FAILED, "WR-MIL-Gateway not running");
  }


  // configure MSI that triggers our irq_handler
  eb_address_t mailbox_base = args.mailbox.sdb_component.addr_first;
  eb_data_t mailbox_value;
  unsigned slot = 0;

  device.read(mailbox_base, EB_DATA32, &mailbox_value);
  while((mailbox_value != 0xffffffff) && slot < 128) {
    device.read(mailbox_base + slot * 4 * 2, EB_DATA32, &mailbox_value);
    // std::cerr << "looking for free slot("<<slot<<"): mailbox_value = 0x" 
    //           << std::hex << std::setw(8) << std::setfill('0') << mailbox_value 
    //           << std::dec << std::endl;
    slot++;
  }

  if (slot < 128)
    mbx_slot = --slot;
  else
    clog << kLogErr << "WrMilGateway: no free slot in mailbox left"  << std::endl;

  //save the irq address in the odd mailbox slot
  //keep postal address to free later
  mailbox_slot_address = mailbox_base + slot * 4 * 2 + 4;
  device.write(mailbox_slot_address, EB_DATA32, (eb_data_t)irq);
  // std::cerr << "WrMilGateway: saved irq 0x" << std::hex << irq << " in mailbox slot " << std::dec << slot << "   slot adr 0x" << std::hex << std::setw(8) << std::setfill('0') << mailbox_slot_address << std::endl;
  // done with MSI configuration

  // configure mailbox for MSI to Saftlib
  // get mailbox slot number for swi host=>lm32
  // eb_data_t mb_slot;
  // mb_slot = readRegisterContent(WR_MIL_GW_REG_MSI_SLOT);
  // std::cerr << "mb_slot = " << mb_slot << std::endl;

  // tell the LM32 which slot in mailbox is ours
  writeRegisterContent(WR_MIL_GW_REG_MSI_SLOT, slot);
  // std::cerr << "lm32 slot configured to = " << slot << std::endl;

  // reset WR-MIL Gateway in case the firmware is already running. 
  //  This prevents resetting the CPU in the middle of a WB cycle
  // reset all other LM32 CPUs to make sure that no other firmware disturbs our actions
  // (in particular the function generator firmware)
  device.sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x3a362063), devices);
  // std::cerr << "WrMilGateway: reset the CPUs" << std::endl;
  // writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_RESET);
  // std::cerr << "WrMilGateway: put firmware in reset state" << std::endl;

/*  for (auto reset_device: devices) {
    // use the first reset device
    uint32_t set_bits = ~(1<<cpu_idx) & 0xff;
    //uint32_t clr_bits =  (1<<cpu_idx);
    // reset register offsets
    //0x4 -> GET 
    //0x8 -> SET 
    //0xc -> CLR
    const int SET = 0x8;
    //std::cerr << "set_bits = 0x" << std::hex << std::setfill('0') << std::setw(8) << set_bits << std::endl;
    //std::cerr << "clr_bits = 0x" << std::hex << std::setfill('0') << std::setw(8) << clr_bits << std::endl;
    device.write(reset_device.sdb_component.addr_first + SET, 
                          EB_DATA32, 
                          set_bits);
    break;
  } */

  //std::cerr << "WrMilGateway: check if firmware is running" << std::endl;
  firmware_running = firmwareRunning();
  firmware_state   = readRegisterContent(WR_MIL_GW_REG_STATE);
  event_source     = readRegisterContent(WR_MIL_GW_REG_EVENT_SOURCE);
  num_late_events  = readRegisterContent(WR_MIL_GW_REG_LATE_EVENTS);

  
  // poll some registers periodically
  pollConnection = Slib::signal_timeout().connect(sigc::mem_fun(*this, &WrMilGateway::poll), poll_period);
}

WrMilGateway::~WrMilGateway()
{

  writeRegisterContent(WR_MIL_GW_REG_MSI_SLOT, 0xffffffff);

  // std::cerr << "WrMilGateway::~WrMilGateway()" << std::endl;
  // clean the mailbox
  device.write(mailbox_slot_address, EB_DATA32, 0xffffffff);
  device.write(mailbox_slot_address-4, EB_DATA32, 0xffffffff);
  //eb_data_t mailbox_value;
  // device.read(mailbox_slot_address, EB_DATA32, &mailbox_value);
  // std::cerr << "WrMilGateway: saved irq 0x" << std::hex << mailbox_value 
  //           << "   slot adr 0x" << std::hex << std::setw(8) << std::setfill('0') << mailbox_slot_address << std::endl;
  // device.read(mailbox_slot_address-4, EB_DATA32, &mailbox_value);
  // std::cerr << "WrMilGateway: saved irq 0x" << std::hex << mailbox_value 
  //           << "   slot adr 0x" << std::hex << std::setw(8) << std::setfill('0') << mailbox_slot_address-4 << std::endl;

/*  // clear all CPU reset bits (as they were before)
  std::vector<struct sdb_device> devices;
  device.sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x3a362063), devices);
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_RESET);
  for (auto reset_device: devices) {
    const int CLR = 0xc;
    device.write(reset_device.sdb_component.addr_first + CLR, 
                          EB_DATA32, 
                          0xffffffff);
    break;
  }*/

  pollConnection.disconnect();
  device.release_irq(irq);

}

bool WrMilGateway::firmwareRunning() const
{
  // std::cerr << "WrMilGateway::firmwareRunning()" << std::endl;
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
  // std::cerr << "WrMilGateway::getFirmwareRunning()" << std::endl;
  bool new_firmware_running = firmwareRunning();

  // emit signal on change
  if (new_firmware_running != firmware_running) {
    firmware_running = new_firmware_running;
    SigFirmwareRunning(firmware_running);
  }

  return firmware_running;
}


uint32_t WrMilGateway::readRegisterContent(uint32_t reg_offset) const
{
  // std::cerr << "WrMilGateway::readRegisterContent()" << std::endl;
  eb_data_t value;
  device.read(base_addr + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, &value);
  return value;
}

void WrMilGateway::writeRegisterContent(uint32_t reg_offset, uint32_t value)
{
  // std::cerr << "WrMilGateway::writeRegisterContent()" << std::endl;
  device.write(base_addr + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, (eb_data_t)value);
}

std::shared_ptr<WrMilGateway> WrMilGateway::create(const ConstructorType& args)
{
  // std::cerr << "WrMilGateway::create()" << std::endl;
  return RegisteredObject<WrMilGateway>::create(args.objectPath, args);
}

std::vector< uint32_t > WrMilGateway::getRegisterContent() const
{
  etherbone::Cycle cycle;
  std::vector<uint32_t> registerContent((WR_MIL_GW_REG_LATE_HISTOGRAM-WR_MIL_GW_REG_MAGIC_NUMBER) / 4, 42);
  uint32_t reg_idx = 0;
  for (auto &reg: registerContent) {
    reg = readRegisterContent(reg_idx);
    reg_idx += 4;
  }
  return registerContent;
}

std::vector< uint32_t > WrMilGateway::getMilHistogram() const
{
  // std::cerr << "WrMilGateway::getMilHistogram()" << std::endl;
  std::vector< uint32_t > histogram(256,0);
  for (unsigned i = 0; i < histogram.size(); ++i) {
    histogram[i] = readRegisterContent(WR_MIL_GW_REG_MIL_HISTOGRAM + 4*i);
  }
  return histogram;
}

bool WrMilGateway::getInUse() const
{
  // std::cerr << "WrMilGateway::getInUse()" << std::endl;
  return time_without_mil_events <= max_time_without_mil_events;
}


// the poll function determines status information that cannot be
// delivered by interrupts: if the firmware is running and if the 
// number of translated event increases (i.e. the gateway is 
// actively used). 
bool WrMilGateway::poll()
{
  // std::cerr << "WrMilGateway::poll()" << std::endl;
  getFirmwareRunning();

  // these three checks are done on MSI base now (no polling needed)
  // getFirmwareState();
  // getEventSource();
  // getNumLateMilEvents();

  // check if the gateway is used (translates events)
  uint64_t new_num_mil_events = getNumMilEvents();
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
      //  every second in normal operation.
      // This signal is only produced only once
      //  if the status changes.
      SigInUse(false);
    }
  }

  return true; // return true to continue polling
}

void WrMilGateway::StartSIS18()
{
  // std::cerr << "WrMilGateway::StartSIS18()" << std::endl;
  // configure WR-MIL Gateway firmware to start operation as SIS18 Pulszentrale
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_SIS);
  clog << kLogInfo << "WR-MIL-Gateway: configured as SIS18 Pulszentrale" << std::endl;
}
void WrMilGateway::StartESR()
{
  // std::cerr << "WrMilGateway::StartESR()" << std::endl;
  // configure WR-MIL Gateway firmware to start operation as ESR Pulszentrale
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_ESR);
  clog << kLogInfo << "WR-MIL-Gateway: configured as ESR Pulszentrale" << std::endl;
}
void WrMilGateway::ClearStatistics()
{
  // std::cerr << "WrMilGateway::ClearStatistics()" << std::endl;
  for (int i = 0; i < (WR_MIL_GW_REG_MIL_HISTOGRAM-WR_MIL_GW_REG_NUM_EVENTS_HI)/4+256; ++i) {
    writeRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_HI + i*4, 0x0);
  }
}

void WrMilGateway::ResetGateway()
{
  // std::cerr << "WrMilGateway::ResetGateway()" << std::endl;
  ClearStatistics();
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_RESET);
}
void WrMilGateway::KillGateway()
{
  // std::cerr << "WrMilGateway::KillGateway()" << std::endl;
  ClearStatistics();
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_KILL);
}


uint32_t WrMilGateway::getWrMilMagic() const
{
  // std::cerr << "WrMilGateway::getWrMilMagic()" << std::endl;

  return readRegisterContent(WR_MIL_GW_REG_MAGIC_NUMBER);
}
uint32_t WrMilGateway::getFirmwareState() const
{
  // std::cerr << "WrMilGateway::getFirmwareState()" << std::endl;

  auto new_firmware_state = readRegisterContent(WR_MIL_GW_REG_STATE);
  if (firmware_state != new_firmware_state) {
    firmware_state = new_firmware_state;
    SigFirmwareState(firmware_state);
    // in case the firmware state has changed 
    // also check for the event source configuration
    getEventSource(); 
  }
  return firmware_state;
}
uint32_t WrMilGateway::getEventSource() const
{
  // std::cerr << "WrMilGateway::getEventSource()" << std::endl;

  auto new_event_source = readRegisterContent(WR_MIL_GW_REG_EVENT_SOURCE);
  if (event_source != new_event_source) {
    event_source = new_event_source;
    SigEventSource(event_source);
  }
  return event_source;
}
unsigned char WrMilGateway::getUtcTrigger() const
{
  // std::cerr << "WrMilGateway::getUtcTrigger()" << std::endl;

  return readRegisterContent(WR_MIL_GW_REG_UTC_TRIGGER);
}
uint32_t WrMilGateway::getEventLatency() const
{
  // std::cerr << "WrMilGateway::getEventLatency()" << std::endl;
  return readRegisterContent(WR_MIL_GW_REG_LATENCY);
}
uint32_t WrMilGateway::getUtcUtcDelay() const
{
  // std::cerr << "WrMilGateway::getUtcUtcDelay()" << std::endl;

  return readRegisterContent(WR_MIL_GW_REG_UTC_DELAY);
}
uint32_t WrMilGateway::getTriggerUtcDelay() const
{
  // std::cerr << "WrMilGateway::getTriggerUtcDelay()" << std::endl;

  return readRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY);
}
uint64_t WrMilGateway::getUtcOffset() const
{
    // std::cerr << "WrMilGateway::getUtcOffset()" << std::endl;

  uint64_t result = readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO);
  return result;
}
uint64_t WrMilGateway::getNumMilEvents() const
{
    // std::cerr << "WrMilGateway::getNumMilEvents()" << std::endl;

  uint64_t result = readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_LO);
  return result;
}
uint32_t WrMilGateway::getNumLateMilEvents() const
{
    // std::cerr << "WrMilGateway::getNumMilEvents()" << std::endl;

  uint32_t new_num_late_events = readRegisterContent(WR_MIL_GW_REG_LATE_EVENTS);
  if (num_late_events != new_num_late_events) {
    // send the current number and the ones since last signal
    uint32_t difference = 0;
    if (new_num_late_events >= num_late_events) {
      difference = new_num_late_events - num_late_events;
    }
    SigNumLateMilEvents(new_num_late_events, difference);
    num_late_events = new_num_late_events;
  }
  return num_late_events;
}
std::vector< uint32_t > WrMilGateway::getLateHistogram() const
{
    // std::cerr << "WrMilGateway::getLateHistogram()" << std::endl;

  std::vector<uint32_t> lateHistogram((WR_MIL_GW_REG_MIL_HISTOGRAM-WR_MIL_GW_REG_LATE_HISTOGRAM) / 4, 0);
  for (unsigned i = 0; i < lateHistogram.size(); ++i) {
    lateHistogram[i] = readRegisterContent(WR_MIL_GW_REG_LATE_HISTOGRAM + 4*i);
  }
  return lateHistogram;

}


void WrMilGateway::setUtcTrigger(unsigned char val)
{
    // std::cerr << "WrMilGateway::setUtcTrigger()" << std::endl;

  writeRegisterContent(WR_MIL_GW_REG_UTC_TRIGGER, val);
}
void WrMilGateway::setEventLatency(uint32_t val)
{
    // std::cerr << "WrMilGateway::setEventLatency()" << std::endl;

  writeRegisterContent(WR_MIL_GW_REG_LATENCY, val);
}
void WrMilGateway::setUtcUtcDelay(uint32_t val)
{
    // std::cerr << "WrMilGateway::setUtcUtcDelay()" << std::endl;

  writeRegisterContent(WR_MIL_GW_REG_UTC_DELAY, val);
}
void WrMilGateway::setTriggerUtcDelay(uint32_t val)
{
    // std::cerr << "WrMilGateway::setTriggerUtcDelay()" << std::endl;
  writeRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY, val);
}
void WrMilGateway::setUtcOffset(uint64_t val)
{
    // std::cerr << "WrMilGateway::setUtcOffset()" << std::endl;

  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO, val & 0x00000000ffffffff);
  val >>= 32;
  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_HI, val & 0x00000000ffffffff);
}



void WrMilGateway::Reset() 
{
      // std::cerr << "WrMilGateway::Reset()" << std::endl;
}

void WrMilGateway::ownerQuit()
{
      // std::cerr << "WrMilGateway::ownerQuit()" << std::endl;
}


void WrMilGateway::irq_handler(eb_data_t msg) const 
{
   // std::cerr << "WrMilGateway::irq_handler(" << msg << ") called" << std::endl;

  switch(msg) {
    case WR_MIL_GW_MSI_LATE_EVENT:
      clog << kLogErr << "WR-MIL-Gateway: late MIL event " << std::endl;

        // std::cerr << "WrMilGateway::irq_handler(WR_MIL_GW_MSI_LATE_EVENT)" << std::endl;
      getNumLateMilEvents(); 
    break;
    case WR_MIL_GW_MSI_STATE_CHANGED:
      //clog << kLogInfo << "WR-MIL-Gateway: firmware state changed" << std::endl;
        // std::cerr << "WrMilGateway::irq_handler(WR_MIL_GW_MSI_STATE_CHANGED)" << std::endl;
      switch(getFirmwareState()) {
        case WR_MIL_GW_STATE_INIT:
          clog << kLogInfo << "WR-MIL-Gateway: firmware state changed to INIT" << std::endl;
        break;
        case WR_MIL_GW_STATE_UNCONFIGURED:
          clog << kLogInfo << "WR-MIL-Gateway: firmware state changed to UNCONFIGURED" << std::endl;
        break;
        case WR_MIL_GW_STATE_CONFIGURED:
          clog << kLogInfo << "WR-MIL-Gateway: firmware state changed to CONFIGURED" << std::endl;
        break;
        case WR_MIL_GW_STATE_PAUSED:
          clog << kLogInfo << "WR-MIL-Gateway: firmware state changed to PAUSED" << std::endl;
        break;
        default:
          clog << kLogErr << "WR-MIL-Gateway: firmware state changed to UNKNOWN" << std::endl;
        break;
      }
    break;
    default:;
      // std::cerr << "WrMilGateway::irq_handler() unknown Interrupt: " << std::dec << msg << std::endl; 
  }
  // std::cerr << "WrMilGateway::irq_handler() done" << std::endl;
}




}
