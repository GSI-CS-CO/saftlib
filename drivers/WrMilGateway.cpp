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
   poll_period(100), // [ms]
   max_time_without_mil_events(16000), // 16 seconds
   time_without_mil_events(max_time_without_mil_events),
   device(args.device),
   have_wrmilgw(false),
   idle(false)
{
  // get all LM32 devices and see if any of them has the WR-MIL-Gateway magic number
  std::vector<struct sdb_device> lm32_devices;
  device.sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x54111351), lm32_devices);
  int cpu_idx = 0;
  for(auto lm32_ram_user: lm32_devices) {
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
  // reset all other LM32 CPUs to make sure that no other firmware disturbs our actions
  // (in particular the function generator firmware)
  std::vector<struct sdb_device> reset_devices;
  device.sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x3a362063), reset_devices);
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_RESET);

  for (auto reset_device: reset_devices) {
    // use the first reset device
    uint32_t all_cpus = (1<<lm32_devices.size())-1;
    uint32_t set_bits = ~(1<<cpu_idx) & all_cpus;
    // reset register offsets
    //0x4 -> GET 
    //0x8 -> SET 
    //0xc -> CLR
    const int SET = 0x8;
    device.write(reset_device.sdb_component.addr_first + SET, 
                          EB_DATA32, 
                          set_bits);
    break;
  } 

  firmware_running = firmwareRunning();
  firmware_state   = readRegisterContent(WR_MIL_GW_REG_STATE);
  event_source     = readRegisterContent(WR_MIL_GW_REG_EVENT_SOURCE);
  num_late_events  = readRegisterContent(WR_MIL_GW_REG_LATE_EVENTS);

  // poll some registers periodically
  pollConnection = Slib::signal_timeout().connect(sigc::mem_fun(*this, &WrMilGateway::poll), poll_period);

  // find oled device
  uint64_t OLED_SDB_VENDOR_ID = UINT64_C(0x651);
  uint32_t OLED_SDB_DEVICE_ID = UINT32_C(0x93a6f3c4);

  // OLED_RESET_OWR    0x0   //wo,  1 b, Resets the OLED display
  // OLED_COL_OFFS_RW  0x4   //rw,  8 b, first visible pixel column. 0x23 for old, 0x30 for new controllers. default is 0x30
  // OLED_UART_OWR     0x8   //wo,  8 b, UART input FIFO. Ascii b7..0
  // OLED_CHAR_OWR     0xc   //wo, 20 b, Char input FIFO. Row b14..12, Col b11..8, Ascii b7..0
  // OLED_RAW_OWR      0x10  //wo, 20 b, Raw  input FIFO. Disp RAM Adr b18..8, Pixel (Col) b7..0
  std::vector<struct sdb_device> oled_devices;
  device.sdb_find_by_identity(OLED_SDB_VENDOR_ID, OLED_SDB_DEVICE_ID, oled_devices);
  if (!oled_devices.empty()) {
    oled_reset = oled_devices.front().sdb_component.addr_first + 0x0;
    oled_char = oled_devices.front().sdb_component.addr_first + 0xc;
  }
  device.write(oled_reset, EB_DATA32, (eb_data_t)1); // reset the oled
  oledUpdate();

  // find MIL piggy device
  uint64_t MIL_SDB_VENDOR_ID = UINT64_C(0x651);
  uint32_t MIL_SDB_DEVICE_ID = UINT32_C(0x35aa6b96);
  std::vector<struct sdb_device> mil_devices;
  device.sdb_find_by_identity(MIL_SDB_VENDOR_ID, MIL_SDB_DEVICE_ID, mil_devices);
  if (!oled_devices.empty()) {
    mil_events_present     = mil_devices.front().sdb_component.addr_first + 0x1008; 
    mil_event_read_and_pop = mil_devices.front().sdb_component.addr_first + 0x1014;
  }

}

void WrMilGateway::oledUpdate()
{
  std::ostringstream lines[6];

  lines[0] << "WRMIL ";
  switch(event_source) {
    case WR_MIL_GW_EVENT_SOURCE_SIS:
      lines[0] << "SIS18";
    break;
    case WR_MIL_GW_EVENT_SOURCE_ESR:
      lines[0] << "ESR";
    break;
    default:
      lines[0] << "???";
  }

  if (readRegisterContent(WR_MIL_GW_REG_SET_OP_READY)) {
    lines[1] << "OP_READY";
  } 

  lines[2] << "#"     << std::setw(9) << num_mil_events;
  //lines[3] << "#late" << std::setw(5)  << num_late_events;

  
  if (idle) {
    lines[5] << "IDLE";
  }

  etherbone::Cycle cycle;
  cycle.open(device);
  for (unsigned row = 0; row < 6; ++row) {
    std::string line = lines[row].str();
    for (unsigned col = 0; col < 11; ++col) {
      char ch = ' ';
      if (col < line.size()) {
        ch = line[col];
      }
      cycle.write(oled_char, EB_DATA32, (eb_data_t)((row<<12) | (col<<8) | ch));
    }
  }
  cycle.close();
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
  eb_data_t value;
  device.read(base_addr + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, &value);
  return value;
}

void WrMilGateway::writeRegisterContent(uint32_t reg_offset, uint32_t value)
{
  device.write(base_addr + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, (eb_data_t)value);
}

std::shared_ptr<WrMilGateway> WrMilGateway::create(const ConstructorType& args)
{
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
  return !idle;
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
  firmware_state   = readRegisterContent(WR_MIL_GW_REG_STATE);
  event_source     = readRegisterContent(WR_MIL_GW_REG_EVENT_SOURCE);
  num_late_events  = readRegisterContent(WR_MIL_GW_REG_LATE_EVENTS);

  // check if the gateway is used (translates events)
  uint64_t new_num_mil_events = getNumMilEvents();
  if (num_mil_events != new_num_mil_events) {
    if (idle) {
      // in this case we change back to being "in use"
      idle = false;
      SigInUse(true);
    }
    time_without_mil_events = 0;
    num_mil_events = new_num_mil_events;
  } else {
    time_without_mil_events += poll_period;
  }

  if (time_without_mil_events >= max_time_without_mil_events/2) {
    RequestFillEvent();
    time_without_mil_events = 0;
    if (!idle) {
      SigInUse(false);
      idle = true;
    }
  } 

  oledUpdate();

  // look for MIl events caputred by the MIL piggy;
  for (;;) {
    eb_data_t value;
    device.read(mil_events_present, EB_DATA32, &value);
    if (value & 0x8) {
      // we have mil events
      device.read(mil_event_read_and_pop, EB_DATA32, &value);
      SigReceivedMilEvent(value); // send signal to Proxies
    } else {
      break;
    }
  }

  return true; // return true to continue polling
}

void WrMilGateway::StartSIS18()
{
  // configure WR-MIL Gateway firmware to start operation as SIS18 Pulszentrale
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_SIS);
  clog << kLogInfo << "WR-MIL-Gateway: configured as SIS18 Pulszentrale" << std::endl;
}
void WrMilGateway::StartESR()
{
  // configure WR-MIL Gateway firmware to start operation as ESR Pulszentrale
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_ESR);
  clog << kLogInfo << "WR-MIL-Gateway: configured as ESR Pulszentrale" << std::endl;
}
void WrMilGateway::ClearStatistics()
{
  for (int i = 0; i < (WR_MIL_GW_REG_MIL_HISTOGRAM-WR_MIL_GW_REG_NUM_EVENTS_HI)/4+256; ++i) {
    writeRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_HI + i*4, 0x0);
  }
}

void WrMilGateway::ResetGateway()
{
  ClearStatistics();
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_RESET);
}
void WrMilGateway::KillGateway()
{
  ClearStatistics();
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_KILL);
}
void WrMilGateway::UpdateOLED()
{
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_UPDATE_OLED);
}

void WrMilGateway::RequestFillEvent()
{
  writeRegisterContent(WR_MIL_GW_REG_REQUEST_FILL_EVT, 1);
}


uint32_t WrMilGateway::getWrMilMagic() const
{
  return readRegisterContent(WR_MIL_GW_REG_MAGIC_NUMBER);
}
uint32_t WrMilGateway::getFirmwareState() const
{
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
uint32_t WrMilGateway::getEventLatency() const
{
  return readRegisterContent(WR_MIL_GW_REG_LATENCY);
}
uint32_t WrMilGateway::getUtcUtcDelay() const
{
  return readRegisterContent(WR_MIL_GW_REG_UTC_DELAY);
}
uint32_t WrMilGateway::getTriggerUtcDelay() const
{
  return readRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY);
}
uint64_t WrMilGateway::getUtcOffset() const
{
  uint64_t result = readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO);
  return result;
}
uint64_t WrMilGateway::getNumMilEvents() const
{
  uint64_t result = readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_LO);
  return result;
}
uint32_t WrMilGateway::getNumLateMilEvents() const
{
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
  std::vector<uint32_t> lateHistogram((WR_MIL_GW_REG_MIL_HISTOGRAM-WR_MIL_GW_REG_LATE_HISTOGRAM) / 4, 0);
  for (unsigned i = 0; i < lateHistogram.size(); ++i) {
    lateHistogram[i] = readRegisterContent(WR_MIL_GW_REG_LATE_HISTOGRAM + 4*i);
  }
  return lateHistogram;
}


void WrMilGateway::setUtcTrigger(unsigned char val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_TRIGGER, val);
}
void WrMilGateway::setEventLatency(uint32_t val)
{
  writeRegisterContent(WR_MIL_GW_REG_LATENCY, val);
}
void WrMilGateway::setUtcUtcDelay(uint32_t val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_DELAY, val);
}
void WrMilGateway::setTriggerUtcDelay(uint32_t val)
{
  writeRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY, val);
}
void WrMilGateway::setUtcOffset(uint64_t val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO, val & 0x00000000ffffffff);
  val >>= 32;
  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_HI, val & 0x00000000ffffffff);
}
void WrMilGateway::setOpReady(bool val)
{
  writeRegisterContent(WR_MIL_GW_REG_SET_OP_READY, val);
}


void WrMilGateway::Reset() 
{
      // std::cerr << "WrMilGateway::Reset()" << std::endl;
}

void WrMilGateway::ownerQuit()
{
      // std::cerr << "WrMilGateway::ownerQuit()" << std::endl;
}


}
