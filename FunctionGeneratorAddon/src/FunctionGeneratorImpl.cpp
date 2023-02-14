
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

#include <memory>
#include <functional>
#include <assert.h>

#include <saftbus/error.hpp>

#include <SAFTd.hpp>
#include <TimingReceiver.hpp>

#include <sigc++/sigc++.h>
#include "FunctionGeneratorImpl.hpp"
#include "fg_regs.h"
//#include "clog.h"


// edits for use with master function generator:
// no longer a dbus object
// dbus-signals to proxy changed to sigc-signals to fg/masterfg


namespace saftlib {

FunctionGeneratorImpl::FunctionGeneratorImpl(SAFTd *saft_daemon
                                           , TimingReceiver *timing_receiver
                                           , std::shared_ptr<std::vector<int> > channel_allocation
                                           , eb_address_t fgb
                                           , int mbox_slot
                                           , unsigned n_channels
                                           , unsigned buf_size
                                           , unsigned idx
                                           , uint32_t macro)
 : saftd(saft_daemon),
   tr(timing_receiver),
   mbx(static_cast<Mailbox*>(timing_receiver)),
   device(tr->OpenDevice::get_device()),
   allocation(channel_allocation),
   shm(fgb + SHM_BASE),
   //swi(args.swi),
   mb_slot(mbox_slot),
   //base(args.base),
   num_channels(n_channels),
   buffer_size(buf_size),
   index(idx),
   scubusSlot      ((macro >> 24) & 0xFF),
   deviceNumber    ((macro >> 16) & 0xFF),
   version         ((macro >>  8) & 0xFF),
   outputWindowSize((macro >>  0) & 0xFF),
   irq(saftd->request_irq(*mbx, std::bind(&FunctionGeneratorImpl::irq_handler, this, std::placeholders::_1))),
   channel(-1), enabled(false), armed(false), running(false), abort(false), resetTimeout(),
   startTag(0), executedParameterCount(0), fillLevel(0), filled(0),
   fifo(16384), // a fifo entry is 12 bytes long. 
                // Make the circular buffer large enough so that re-alloction is hopefully not needed 
                // (initial size is 192 KiB for buffer size of 16384)
   fg_fifo_max_size(0)
{
  // DRIVER_LOG("",-1, -1);

  char *fg_fifo_max_size_env = getenv("SAFTLIB_FG_FIFO_MAX_SIZE");
  if (fg_fifo_max_size_env != nullptr) {
    std::istringstream in(fg_fifo_max_size_env);
    long new_size;
    in >> new_size;
    if (!in) {
      // reading new size didn't work
      std::cerr <<  " cannot read initial capacity from environment variable: \'" << fg_fifo_max_size_env << "\'" << std::endl;
    } else {
      std::cerr <<  "set fifo capacity based on environment variable to " << new_size << std::endl;
      fifo.set_capacity(new_size);
    }
  }

  host_slot = mbx->ConfigureSlot(irq);
  // eb_address_t mb_base = args.mbx.sdb_component.addr_first;
  // eb_data_t mb_value;
  // unsigned slot = 0;

  // device.read(mb_base, EB_DATA32, &mb_value);
  // while((mb_value != 0xffffffff) && slot < 128) {
  //   device.read(mb_base + slot * 4 * 2, EB_DATA32, &mb_value);
  //   slot++;
  // }

  // if (slot < 128) {
  //   mbx_slot = --slot;
  // }
  // else {
  //   std::cerr << "FunctionGenerator: no free slot in mailbox left"  << std::endl;
  // }



  //save the irq address in the odd mailbox slot
  //keep postal address to free later
  // mailbox_slot_address = mb_base + slot * 4 * 2 + 4;
  // device.write(mailbox_slot_address, EB_DATA32, (eb_data_t)irq);
}

FunctionGeneratorImpl::~FunctionGeneratorImpl()
{
  std::cerr << "~FunctionGenerator" << std::endl;
  // DRIVER_LOG("",-1, channel);
  // resetTimeout.disconnect(); // do not run ResetFailed
  //device.release_irq(irq);
  // device.write(mailbox_slot_address, EB_DATA32, 0xffffffff);

  saftbus::Loop::get_default().remove(resetTimeout);
  saftd->release_irq(irq);
}

static unsigned wrapping_sub(unsigned a, unsigned b, unsigned buffer_size)
{
  if (a < b) {
    return a - b + buffer_size;
  } else {
    return a - b;
  }
}

static unsigned wrapping_add(unsigned a, unsigned b, unsigned buffer_size)
{
  unsigned c = a + b;
  if (c >= buffer_size) {
    return c - buffer_size;
  } else {
    return c;
  }
}

void FunctionGeneratorImpl::fifo_push_back(const ParameterTuple& tuple)
{
  if (fifo.size() == fifo.capacity()) {
    std::cerr << "FunctionGeneratorImpl: change fifo capacity from " << std::dec << fifo.size() << " to " << 2*fifo.size() << std::endl;
    fifo.set_capacity(fifo.capacity()*2);
  }
  fifo.push_back(tuple);
  if (fg_fifo_max_size < fifo.size()) {
    fg_fifo_max_size = fifo.size();
  }  
}


bool FunctionGeneratorImpl::lowFill() const
{
  // DRIVER_LOG("channel",-1, channel);
  return fifo.size() < buffer_size * 2;
}

void FunctionGeneratorImpl::refill(bool first)
{
  // DRIVER_LOG("channel",-1, channel);
  etherbone::Cycle cycle;
  eb_data_t write_offset_d = 0, read_offset_d = 0;
  
  assert (channel != -1);
  
  eb_address_t regs = shm + FG_REGS_BASE(channel, num_channels);
  
  // if refill is called for the first time, there is no need for checking the offsets. they are 0 
  if (first == false) {
    cycle.open(device);
    cycle.read(regs + FG_WPTR, EB_DATA32, &write_offset_d);
    cycle.read(regs + FG_RPTR, EB_DATA32, &read_offset_d);
    cycle.close();
  }
  
  unsigned write_offset = write_offset_d % buffer_size;
  unsigned read_offset  = read_offset_d  % buffer_size;
  
  unsigned remaining = wrapping_sub(write_offset, read_offset, buffer_size);
  unsigned completed = filled - remaining;
  
  bool wasLow = lowFill();
  for (unsigned i = 0; i < completed && !fifo.empty(); ++i) {
    fillLevel -= fifo.front().duration();
    --filled;
    fifo.pop_front();
  }
  bool amLow = lowFill();
  
  // should we get more data from the user?
  if (amLow && !wasLow) signal_refill.emit();
  
  // our buffers should now agree
  assert (filled == remaining);
  
  unsigned todo = fifo.size() - filled; // # of records not yet on LM32
  unsigned space = buffer_size-1 - filled;  // free space on LM32
  unsigned refill = std::min(todo, space); // add this many records
  
  cycle.open(device);
  for (unsigned i = 0; i < refill; ++i) {
    ParameterTuple& tuple = fifo[filled+i];
    uint32_t coeff_a, coeff_b, coeff_ab, coeff_c, control;
    
    coeff_a = (int32_t)tuple.coeff_a; // sign extended
    coeff_b = (int32_t)tuple.coeff_b;
    coeff_c = tuple.coeff_c;
    
    coeff_a <<= 16;
    coeff_b &= 0xFFFF;
    coeff_ab = coeff_a | coeff_b;
    
    control = 
      ((tuple.step    & 0x7)  <<  0) |
      ((tuple.freq    & 0x7)  <<  3) |
      ((tuple.shift_b & 0x3f) <<  6) |
      ((tuple.shift_a & 0x3f) << 12);
    
    unsigned offset = wrapping_add(write_offset, i, buffer_size);
    eb_address_t buff = shm + FG_BUFF_BASE(channel, offset, num_channels, buffer_size);
    cycle.write(buff + PARAM_COEFF_AB, EB_DATA32, coeff_ab);
    cycle.write(buff + PARAM_COEFF_C,  EB_DATA32, coeff_c);
    cycle.write(buff + PARAM_CONTROL,  EB_DATA32, control);
  }
  // update write pointer
  unsigned offset = wrapping_add(write_offset, refill, buffer_size);
  cycle.write(regs + FG_WPTR, EB_DATA32, offset);
  cycle.close();
  
  filled += refill;
}

void FunctionGeneratorImpl::irq_handler(eb_data_t msi)
{
  std::cerr << "FunctionGeneratorImpl::irq_handler " << msi << std::endl;
  // DRIVER_LOG("msi",-1, msi);
  // ignore spurious interrupt
  if (channel == -1) {
    std::cerr << "FunctionGenerator: received unsolicited IRQ on index " << std::dec << index << std::endl;
    return;
  }
  
 switch(msi)
 {
   case IRQ_DAT_REFILL         : std::cerr <<  "FG MSI REFILL ch "         << channel << " index " << index << std::endl; break;
   case IRQ_DAT_START          : std::cerr <<  "FG MSI START ch "          << channel << " index " << index << std::endl; break;
   case IRQ_DAT_STOP_EMPTY     : std::cerr <<  "FG MSI STOP_EMPTY ch "     << channel << " index " << index << std::endl; break;
   case IRQ_DAT_STOP_NOT_EMPTY : std::cerr <<  "FG MSI STOP_NOT_EMPTY ch " << channel << " index " << index << std::endl; break;
   case IRQ_DAT_ARMED          : std::cerr <<  "FG MSI ARMED ch "          << channel << " index " << index << std::endl; break;
   case IRQ_DAT_DISARMED       : std::cerr <<  "FG MSI DISARMED ch "       << channel << " index " << index << std::endl; break;
   default : std::cerr <<  "FG Unexpected MSI " << msi << " ch " << channel << " index " << index << std::endl;
 } 
  // microcontroller cannot break this invariant; we always set channel and enabled together
  assert (enabled);
  
  // !!! imprecise; should be timestamped by the hardware
  uint64_t time = tr->ReadRawCurrentTime();
  
  // make sure the evil microcontroller does not violate message sequencing
  if (msi == IRQ_DAT_REFILL) {
    if (!running) {
      std::cerr << "FunctionGenerator: received refill while not running on index " << std::dec << index << std::endl;
    } else {
      refill(false);
    }
  } else if (msi == IRQ_DAT_ARMED) {
    if (running) {
      std::cerr << "FunctionGenerator: received armed while running on index " << std::dec << index << std::endl;
    } else if (armed) {
      std::cerr << "FunctionGenerator: received armed while armed on index " << std::dec << index << std::endl;
    } else {
      armed = true;
      signal_armed.emit(armed);
    }
  } else if (msi == IRQ_DAT_DISARMED) {
    if (running) {
      std::cerr << "FunctionGenerator: received disarmed while running on index " << std::dec << index << std::endl;
    } else if (!armed) {
      std::cerr << "FunctionGenerator: received disarmed while not armed on index " << std::dec << index << std::endl;
    } else {
      armed = false;
      signal_armed.emit(armed);
      releaseChannel();
    }
  } else if (msi == IRQ_DAT_START) {
    if (running) {
      std::cerr << "FunctionGenerator: received start while running on index " << std::dec << index << std::endl;
    } else if (!armed) {
      std::cerr << "FunctionGenerator: received start while not armed on index " << std::dec << index << std::endl;
    } else {
      armed = false;
      running = true;
      signal_armed.emit(armed);
      signal_running.emit(running);      
      signal_started.emit(time);
    }
  } else { // stopped?
    if (armed) {
      std::cerr << "FunctionGenerator: received stop while armed on index " << std::dec << index << std::endl;
    } else if (!running) {
      std::cerr << "FunctionGenerator: received stop while not running on index " << std::dec << index << std::endl;
    } else {
      bool hardwareMacroUnderflow = (msi != IRQ_DAT_STOP_EMPTY) && !abort;
      bool microControllerUnderflow = fifo.size() != filled && !hardwareMacroUnderflow && !abort;
      if (!abort && !hardwareMacroUnderflow && !microControllerUnderflow) { // success => empty FIFO
        fillLevel = 0;
        fifo.clear();
      }
      executedParameterCount = ReadExecutedParameterCount();
      running = false;
      signal_running.emit(running);
      signal_stopped.emit(time, abort, hardwareMacroUnderflow, microControllerUnderflow);
      releaseChannel();
    }
  }
}

uint64_t ParameterTuple::duration() const
{
  static const uint64_t samples[8] = { // fixed in HDL
    250, 500, 1000, 2000, 4000, 8000, 16000, 32000
  };
  static const uint64_t sample_len[8] = { // fixed in HDL
    62500, // 16kHz in ns
    31250, // 32kHz
    15625, // 64kHz
     8000, // 125kHz
     4000, // 250kHz
     2000, // 500kHz
     1000, // 1GHz
      500  // 2GHz
  };
  return samples[step] * sample_len[freq];
}


bool FunctionGeneratorImpl::appendParameterTuples(std::vector<ParameterTuple> parameters)
{
  // DRIVER_LOG("param.size()",-1, parameters.size());
  for (ParameterTuple p : parameters)
  {
    fifo_push_back(p);
    fillLevel += p.duration();
  }

  if (channel != -1) refill(false);
  return lowFill();
}


bool FunctionGeneratorImpl::appendParameterSet(
  const std::vector< int16_t >& coeff_a,
  const std::vector< int16_t >& coeff_b,
  const std::vector< int32_t >& coeff_c,
  const std::vector< unsigned char >& step,
  const std::vector< unsigned char >& freq,
  const std::vector< unsigned char >& shift_a,
  const std::vector< unsigned char >& shift_b )

{
  // DRIVER_LOG("coeff.size()",-1, coeff_a.size());

  // confirm lengths match
  unsigned len = coeff_a.size();
  
  if (coeff_b.size() != len) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "coeff_b length mismatch");
  if (coeff_c.size() != len) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "coeff_c length mismatch");
  if (step.size()    != len) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "step length mismatch");
  if (freq.size()    != len) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "freq length mismatch");
  if (shift_a.size() != len) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "shift_a length mismatch");
  if (shift_b.size() != len) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "shift_b length mismatch");
  
  // validate data
  for (unsigned i = 0; i < len; ++i) {
    if (step[i] >= 8) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "step must be < 8");
    if (freq[i] >= 8) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "freq must be < 8");
    if (shift_a[i] > 48) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "shift_a must be <= 48");
    if (shift_b[i] > 48) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "shift_b must be <= 48");
  }
  
  // import the data
  for (unsigned i = 0; i < len; ++i) {
    ParameterTuple tuple;
    tuple.coeff_a = coeff_a[i];
    tuple.coeff_b = coeff_b[i];
    tuple.coeff_c = coeff_c[i];
    tuple.step    = step[i];
    tuple.freq    = freq[i];
    tuple.shift_a = shift_a[i];
    tuple.shift_b = shift_b[i];
    fifo_push_back(tuple);
    fillLevel += tuple.duration();
  }
  
  if (channel != -1) refill(false);
  return lowFill();

}

void FunctionGeneratorImpl::flush()
{
  // DRIVER_LOG("channel",-1,channel);
  if (enabled) {
    // DRIVER_LOG("enabled_flush_impossible",-1,-1);
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Enabled, cannot Flush");
  }
    
  assert (channel == -1);
  fillLevel = 0;
  fifo.clear();
}

void FunctionGeneratorImpl::Flush()
{
 	flush();  	  
}

uint32_t FunctionGeneratorImpl::getVersion() const
{
  // DRIVER_LOG("channel",-1,channel);
  return version;
}

unsigned char FunctionGeneratorImpl::getSCUbusSlot() const
{
  // DRIVER_LOG("channel",-1,channel);
  return scubusSlot;
}

unsigned char FunctionGeneratorImpl::getDeviceNumber() const
{
  // DRIVER_LOG("channel",-1,channel);
  return deviceNumber;
}

unsigned char FunctionGeneratorImpl::getOutputWindowSize() const
{
  // DRIVER_LOG("channel",-1,channel);
  return outputWindowSize;
}

bool FunctionGeneratorImpl::getEnabled() const
{
  // DRIVER_LOG("channel",-1,channel);
  // DRIVER_LOG("enabled",-1,enabled);
  return enabled;
}

bool FunctionGeneratorImpl::getArmed() const
{
  // DRIVER_LOG("channel",-1,channel);
  // DRIVER_LOG("armed",-1,armed);
  return armed;
}

bool FunctionGeneratorImpl::getRunning() const
{
  // DRIVER_LOG("channel",-1,channel);
  // DRIVER_LOG("running",-1,running);
  return running;
}

uint32_t FunctionGeneratorImpl::getStartTag() const
{
  // DRIVER_LOG("channel",-1,channel);
  // DRIVER_LOG("startTag",-1,startTag);
  return startTag;
}

uint64_t FunctionGeneratorImpl::ReadFillLevel()
{
  // DRIVER_LOG("channel",-1,channel);
  // DRIVER_LOG("fillLevel",-1,fillLevel);
  return fillLevel;
}

uint32_t FunctionGeneratorImpl::ReadExecutedParameterCount()
{
  // DRIVER_LOG("channel",-1,channel);
  if (running) {
    eb_data_t count;
    eb_address_t regs = shm + FG_REGS_BASE(channel, num_channels);
    device.read(regs + FG_RAMP_COUNT, EB_DATA32, &count);
    // DRIVER_LOG("running_count",-1,count);
    return count;
  } else {
    // DRIVER_LOG("notrunning_count",-1,executedParameterCount);
    return executedParameterCount;
  }
}

void FunctionGeneratorImpl::acquireChannel()
{
  // DRIVER_LOG("",-1,-1);
  assert (channel == -1);
  
  unsigned i;
  for (i = 0; i < allocation->size(); ++i)
    if (allocation->operator[](i) == -1) break;
  
  if (i == allocation->size()) {
    std::ostringstream str;
    str.imbue(std::locale("C"));
    str << "All " << allocation->size() << " microcontroller channels are in use";
    throw saftbus::Error(saftbus::Error::FAILED, str.str());
  }
 
  // if this throws, it is not a problem
  eb_address_t regs = shm + FG_REGS_BASE(i, num_channels);
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(regs + FG_WPTR,       EB_DATA32, 0);
  cycle.write(regs + FG_RPTR,       EB_DATA32, 0);
  cycle.write(regs + FG_MBX_SLOT,   EB_DATA32, host_slot->getIndex());
  cycle.write(regs + FG_MACRO_NUM,  EB_DATA32, index);
  cycle.write(regs + FG_RAMP_COUNT, EB_DATA32, 0);
  cycle.write(regs + FG_TAG,        EB_DATA32, startTag);
  cycle.close();
 
  allocation->operator[](i) = index;
  channel = i;
  enabled = true;
  // DRIVER_LOG("got_channel",-1,channel);
  signal_enabled.emit(enabled);
}

void FunctionGeneratorImpl::releaseChannel()
{
  // DRIVER_LOG("channel",-1,channel);
  // int previous_channel = channel;
  assert (channel != -1);
  //resetTimeout.disconnect();
  saftbus::Loop::get_default().remove(resetTimeout);
  if (abort) {
    // DRIVER_LOG("reset_firmware_channel",-1,channel);
    mbx->UseSlot(mb_slot, SWI_INIT_BUFFERS | channel);
  }
  allocation->operator[](channel) = -1;
  channel = -1;
  filled = 0;
  enabled = false;
  signal_enabled.emit(enabled);
  if (abort) {
    // DRIVER_LOG("flush_channel",-1,previous_channel);
    flush();
  }
}

void FunctionGeneratorImpl::arm()
{
  // DRIVER_LOG("channel",-1,channel);
  if (enabled) {
    // DRIVER_LOG("throw_enabled_cannot_rearm_channel",-1,channel);
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Enabled, cannot re-Arm");
  }
  if (fillLevel == 0) {
    // DRIVER_LOG("throw_filllevel_zero_channel",-1,channel);
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "FillLevel is zero, cannot Arm");
  }
  
  // !enabled, so:
  assert(!armed);
  assert(!running);
  
  // actually enable it
  abort = false;
  acquireChannel();
  try {
    // DRIVER_LOG("refill_channel",-1,channel);
    refill(true);
    mbx->UseSlot(mb_slot, SWI_ENABLE | channel);
  } catch (...) {
    // DRIVER_LOG("refill_failed_channel",-1,channel);
    std::cerr << "FunctionGenerator: failed to fill buffers and send enabled SWI on channel " << std::dec << channel << " for index " << index << std::endl;
    mbx->UseSlot(mb_slot, SWI_DISABLE | channel);
    throw;
  }
}

void FunctionGeneratorImpl::Arm()
{
  // DRIVER_LOG("channel",-1,channel);
  arm();
}

bool FunctionGeneratorImpl::ResetFailed()
{
  // DRIVER_LOG("channel",-1,channel);
  assert(enabled);
  std::cerr << "FunctionGenerator: failed to reset on index " << std::dec << index << std::endl;
  
  if (running) { // synthesize missing Stopped
    running = false;
    signal_running.emit(running);
    signal_stopped.emit(tr->ReadRawCurrentTime(), true, false, false);
  } else {
    // synthesize any missing Armed transitions
    if (!armed) {
      armed = true;
      signal_armed.emit(armed);
    }
    armed = false;
    signal_armed.emit(armed);
  }
  releaseChannel();
  return false;
}

void FunctionGeneratorImpl::Reset()
{
  // DRIVER_LOG("channel",-1,channel);
  if (channel == -1) return; // nothing to abort
  if (abort) {
    // DRIVER_LOG("abort alreay in progress",-1,channel);
    return; // abort alreay in progress
  }
  abort = true;
  mbx->UseSlot(mb_slot, SWI_DISABLE | channel);
  // expect disarm or started+stopped, but if not ... timeout:
  if (!resetTimeout.connected()) {
    // resetTimeout = Slib::signal_timeout().connect(
    //   sigc::mem_fun(*this, &FunctionGeneratorImpl::ResetFailed), 1000); // 1 sec
    resetTimeout = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>
      (std::bind(&FunctionGeneratorImpl::ResetFailed, this), std::chrono::milliseconds(1000), std::chrono::milliseconds(1000));
  }
}

void FunctionGeneratorImpl::Abort()
{
  // DRIVER_LOG("channel",-1,channel);
  Reset();
}

void FunctionGeneratorImpl::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  // DRIVER_LOG("channel",-1,channel);
  Reset();
}

void FunctionGeneratorImpl::setStartTag(uint32_t val)
{
  // DRIVER_LOG("channel",-1,channel);
  if (enabled)
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Enabled, cannot set StartTag");
  
  if (val != startTag) {
    startTag = val;
  }
}

std::string FunctionGeneratorImpl::GetName()
{
  // DRIVER_LOG("channel",-1,channel);
    std::ostringstream ss;
    ss << "fg-" << (int)getSCUbusSlot() << "-" << (int)getDeviceNumber();
//    ss << "-" << index ;
    return ss.str();	
}	

}
