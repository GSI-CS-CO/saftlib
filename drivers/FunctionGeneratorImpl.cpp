
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

#include <assert.h>

#include <etherbone.h>
#include <giomm.h>
#include "FunctionGeneratorImpl.h"
#include "TimingReceiver.h"
#include "fg_regs.h"
#include "clog.h"


// edits for use with master function generator:
// no longer a dbus object
// dbus-signals to proxy changed to sigc-signals to fg/masterfg


namespace saftlib {

FunctionGeneratorImpl::FunctionGeneratorImpl(const ConstructorType& args)
 : 
   dev(args.dev),
   allocation(args.allocation),
   shm(args.fgb + SHM_BASE),
   swi(args.swi),
   base(args.base),
   mbx(args.mbx),
   num_channels(args.num_channels),
   buffer_size(args.buffer_size),
   index(args.index),
   scubusSlot      ((args.macro >> 24) & 0xFF),
   deviceNumber    ((args.macro >> 16) & 0xFF),
   version         ((args.macro >>  8) & 0xFF),
   outputWindowSize((args.macro >>  0) & 0xFF),
   irq(args.dev->getDevice().request_irq(args.base, sigc::mem_fun(*this, &FunctionGeneratorImpl::irq_handler))),
   channel(-1), enabled(false), armed(false), running(false), abort(false), resetTimeout(),
   startTag(0), executedParameterCount(0), fillLevel(0), filled(0)
{
  eb_address_t mb_base = args.mbx.sdb_component.addr_first;
  eb_data_t mb_value;
  unsigned slot = 0;

  dev->getDevice().read(mb_base, EB_DATA32, &mb_value);
  while((mb_value != 0xffffffff) && slot < 128) {
    dev->getDevice().read(mb_base + slot * 4 * 2, EB_DATA32, &mb_value);
    slot++;
  }

  if (slot < 128)
    mbx_slot = --slot;
  else
    clog << kLogErr << "FunctionGenerator: no free slot in mailbox left"  << std::endl;

  //save the irq address in the odd mailbox slot
  //keep postal address to free later
  mailbox_slot_address = mb_base + slot * 4 * 2 + 4;
  dev->getDevice().write(mailbox_slot_address, EB_DATA32, (eb_data_t)irq);
  clog << kLogDebug << "FunctionGenerator: saved irq 0x" << std::hex << irq << " in mailbox slot " << std::dec << slot << std::endl;
}

FunctionGeneratorImpl::~FunctionGeneratorImpl()
{
  resetTimeout.disconnect(); // do not run ResetFailed
  dev->getDevice().release_irq(irq);
  
  clog << "FunctionGenerator: freeing mailbox slot " << std::dec << mbx_slot << std::endl;
  dev->getDevice().write(mailbox_slot_address, EB_DATA32, 0xffffffff);
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

bool FunctionGeneratorImpl::lowFill() const
{
  return fifo.size() < buffer_size * 2;
}

void FunctionGeneratorImpl::refill()
{
  etherbone::Cycle cycle;
  eb_data_t write_offset_d, read_offset_d;
  
  assert (channel != -1);
  
  eb_address_t regs = shm + FG_REGS_BASE(channel, num_channels);
  
  cycle.open(dev->getDevice());
  cycle.read(regs + FG_WPTR, EB_DATA32, &write_offset_d);
  cycle.read(regs + FG_RPTR, EB_DATA32, &read_offset_d);
  cycle.close();
  
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
  
  cycle.open(dev->getDevice());
  for (unsigned i = 0; i < refill; ++i) {
    ParameterTuple& tuple = fifo[filled+i];
    guint32 coeff_a, coeff_b, coeff_ab, coeff_c, control;
    
    coeff_a = (gint32)tuple.coeff_a; // sign extended
    coeff_b = (gint32)tuple.coeff_b;
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
  // ignore spurious interrupt
  if (channel == -1) {
    clog << kLogErr << "FunctionGenerator: received unsolicited IRQ on index " << std::dec << index << std::endl;
    return;
  }
  /*
 switch(msi)
 {
   case IRQ_DAT_REFILL : clog << "FG MSI REFILL ch " << channel << " index " << index << std::endl; break;
   case IRQ_DAT_START : clog << "FG MSI START ch " << channel << " index " << index << std::endl; break;
   case IRQ_DAT_STOP_EMPTY : clog << "FG MSI STOP_EMPTY ch " << channel << " index " << index << std::endl; break;
   case IRQ_DAT_STOP_NOT_EMPTY : clog << "FG MSI STOP_NOT_EMPTY ch " << channel << " index " << index << std::endl; break;
   case IRQ_DAT_ARMED : clog << "FG MSI ARMED ch " << channel << " index " << index << std::endl; break;
   case IRQ_DAT_DISARMED : clog << "FG MSI DISARMED ch " << channel << " index " << index << std::endl; break;
   default : clog << "FG Unexpected MSI " << msi << " ch " << channel << " index " << index << std::endl;
 } */
  // microcontroller cannot break this invariant; we always set channel and enabled together
  assert (enabled);
  
  // !!! imprecise; should be timestamped by the hardware
  guint64 time = dev->ReadRawCurrentTime();
  
  // make sure the evil microcontroller does not violate message sequencing
  if (msi == IRQ_DAT_REFILL) {
    if (!running) {
      clog << kLogErr << "FunctionGenerator: received refill while not running on index " << std::dec << index << std::endl;
    } else {
      refill();
    }
  } else if (msi == IRQ_DAT_ARMED) {
    if (running) {
      clog << kLogErr << "FunctionGenerator: received armed while running on index " << std::dec << index << std::endl;
    } else if (armed) {
      clog << kLogErr << "FunctionGenerator: received armed while armed on index " << std::dec << index << std::endl;
    } else {
      armed = true;
      signal_armed.emit(armed);
    }
  } else if (msi == IRQ_DAT_DISARMED) {
    if (running) {
      clog << kLogErr << "FunctionGenerator: received disarmed while running on index " << std::dec << index << std::endl;
    } else if (!armed) {
      clog << kLogErr << "FunctionGenerator: received disarmed while not armed on index " << std::dec << index << std::endl;
    } else {
      armed = false;
      signal_armed.emit(armed);
      releaseChannel();
    }
  } else if (msi == IRQ_DAT_START) {
    if (running) {
      clog << kLogErr << "FunctionGenerator: received start while running on index " << std::dec << index << std::endl;
    } else if (!armed) {
      clog << kLogErr << "FunctionGenerator: received start while not armed on index " << std::dec << index << std::endl;
    } else {
      armed = false;
      running = true;
      signal_armed.emit(armed);
      signal_running.emit(running);      
      signal_started.emit(time);
    }
  } else { // stopped?
    if (armed) {
      clog << kLogErr << "FunctionGenerator: received stop while armed on index " << std::dec << index << std::endl;
    } else if (!running) {
      clog << kLogErr << "FunctionGenerator: received stop while not running on index " << std::dec << index << std::endl;
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

guint64 FunctionGeneratorImpl::ParameterTuple::duration() const
{
  static const guint64 samples[8] = { // fixed in HDL
    250, 500, 1000, 2000, 4000, 8000, 16000, 32000
  };
  static const guint64 sample_len[8] = { // fixed in HDL
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


bool FunctionGeneratorImpl::appendParameterSet(
  const std::vector< gint16 >& coeff_a,
  const std::vector< gint16 >& coeff_b,
  const std::vector< gint32 >& coeff_c,
  const std::vector< unsigned char >& step,
  const std::vector< unsigned char >& freq,
  const std::vector< unsigned char >& shift_a,
  const std::vector< unsigned char >& shift_b )

{

  // confirm lengths match
  unsigned len = coeff_a.size();
  
  if (coeff_b.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "coeff_b length mismatch");
  if (coeff_c.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "coeff_c length mismatch");
  if (step.size()    != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "step length mismatch");
  if (freq.size()    != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "freq length mismatch");
  if (shift_a.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_a length mismatch");
  if (shift_b.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_b length mismatch");
  
  // validate data
  for (unsigned i = 0; i < len; ++i) {
    if (step[i] >= 8) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "step must be < 8");
    if (freq[i] >= 8) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "freq must be < 8");
    if (shift_a[i] > 48) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_a must be <= 48");
    if (shift_b[i] > 48) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_b must be <= 48");
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
    fifo.push_back(tuple);
    fillLevel += tuple.duration();
  }
  
  if (channel != -1) refill();
  return lowFill();

}

void FunctionGeneratorImpl::flush()
{
  if (enabled)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Enabled, cannot Flush");
    
  assert (channel == -1);
  
  fillLevel = 0;
  fifo.clear();
}

void FunctionGeneratorImpl::Flush()
{
 	flush();  	  
}

guint32 FunctionGeneratorImpl::getVersion() const
{
  return version;
}

unsigned char FunctionGeneratorImpl::getSCUbusSlot() const
{
  return scubusSlot;
}

unsigned char FunctionGeneratorImpl::getDeviceNumber() const
{
  return deviceNumber;
}

unsigned char FunctionGeneratorImpl::getOutputWindowSize() const
{
  return outputWindowSize;
}

bool FunctionGeneratorImpl::getEnabled() const
{
  return enabled;
}

bool FunctionGeneratorImpl::getArmed() const
{
  return armed;
}

bool FunctionGeneratorImpl::getRunning() const
{
  return running;
}

guint32 FunctionGeneratorImpl::getStartTag() const
{
  return startTag;
}

guint64 FunctionGeneratorImpl::ReadFillLevel()
{
  return fillLevel;
}

guint32 FunctionGeneratorImpl::ReadExecutedParameterCount()
{
  if (running) {
    eb_data_t count;
    eb_address_t regs = shm + FG_REGS_BASE(channel, num_channels);
    dev->getDevice().read(regs + FG_RAMP_COUNT, EB_DATA32, &count);
    return count;
  } else {
    return executedParameterCount;
  }
}

void FunctionGeneratorImpl::acquireChannel()
{
  assert (channel == -1);
  
  unsigned i;
  for (i = 0; i < allocation->indexes.size(); ++i)
    if (allocation->indexes[i] == -1) break;
  
  if (i == allocation->indexes.size()) {
    std::ostringstream str;
    str.imbue(std::locale("C"));
    str << "All " << allocation->indexes.size() << " microcontroller channels are in use";
    throw Gio::DBus::Error(Gio::DBus::Error::FAILED, str.str());
  }
 
  // if this throws, it is not a problem
  eb_address_t regs = shm + FG_REGS_BASE(i, num_channels);
  etherbone::Cycle cycle;
  cycle.open(dev->getDevice());
  cycle.write(regs + FG_WPTR,       EB_DATA32, 0);
  cycle.write(regs + FG_RPTR,       EB_DATA32, 0);
  cycle.write(regs + FG_MBX_SLOT,   EB_DATA32, mbx_slot);
  cycle.write(regs + FG_MACRO_NUM,  EB_DATA32, index);
  cycle.write(regs + FG_RAMP_COUNT, EB_DATA32, 0);
  cycle.write(regs + FG_TAG,        EB_DATA32, startTag);
  cycle.close();
 
  allocation->indexes[i] = index;
  channel = i;
  enabled = true;
  signal_enabled.emit(enabled);
}

void FunctionGeneratorImpl::releaseChannel()
{
  assert (channel != -1);
  
  resetTimeout.disconnect();
  allocation->indexes[channel] = -1;
  channel = -1;
  filled = 0;
  enabled = false;
  signal_enabled.emit(enabled);
}

void FunctionGeneratorImpl::arm()
{
  if (enabled)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Enabled, cannot re-Arm");
  if (fillLevel == 0)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "FillLevel is zero, cannot Arm");
  
  // !enabled, so:
  assert(!armed);
  assert(!running);
  
  // actually enable it
  abort = false;
  acquireChannel();
  try {

    refill();
    dev->getDevice().write(swi, EB_DATA32, SWI_ENABLE | channel);
  } catch (...) {
    clog << kLogErr << "FunctionGenerator: failed to fill buffers and send enabled SWI on channel " << std::dec << channel << " for index " << index << std::endl;
    dev->getDevice().write(swi, EB_DATA32, SWI_DISABLE | channel);
    throw;
  }
}

void FunctionGeneratorImpl::Arm()
{
  arm();
}

bool FunctionGeneratorImpl::ResetFailed()
{
  assert(enabled);
  clog << kLogErr << "FunctionGenerator: failed to reset on index " << std::dec << index << std::endl;
  
  if (running) { // synthesize missing Stopped
    running = false;
    signal_running.emit(running);
    signal_stopped.emit(dev->ReadRawCurrentTime(), true, false, false);
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
  if (channel == -1) return; // nothing to abort
  if (resetTimeout.connected()) return; // reset already in progress
  dev->getDevice().write(swi, EB_DATA32, SWI_DISABLE | channel);
  // expect disarm or started+stopped, but if not ... timeout:
  resetTimeout = Glib::signal_timeout().connect(
    sigc::mem_fun(*this, &FunctionGeneratorImpl::ResetFailed), 250); // 250ms
}

void FunctionGeneratorImpl::Abort()
{
  Reset();
}

void FunctionGeneratorImpl::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  Reset();
}

void FunctionGeneratorImpl::setStartTag(guint32 val)
{
  if (enabled)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Enabled, cannot set StartTag");
  
  if (val != startTag) {
    startTag = val;
  }
}

Glib::ustring FunctionGeneratorImpl::GetName()
{
    std::ostringstream ss;
    ss << "fg-" << (int)getSCUbusSlot() << "-" << (int)getDeviceNumber();
//    ss << "-" << index ;
    return ss.str();	
}	

}
