#define ETHERBONE_THROWS 1

#include <assert.h>

#include "RegisteredObject.h"
#include "FunctionGenerator.h"
#include "TimingReceiver.h"
#include "fg_regs.h"

namespace saftlib {

FunctionGenerator::FunctionGenerator(ConstructorType args)
 : dev(args.dev), allocation(args.allocation), shm(args.fgb + SHM_BASE), swi(args.swi), 
   num_channels(args.num_channels), buffer_size(args.buffer_size), index(args.index),
   scubusSlot      ((args.macro >> 24) & 0xFF),
   deviceNumber    ((args.macro >> 16) & 0xFF),
   version         ((args.macro >>  8) & 0xFF),
   outputWindowSize((args.macro >>  0) & 0xFF),
   irq(args.dev->getDevice().request_irq(sigc::mem_fun(*this, &FunctionGenerator::irq_handler))),
   busyConnection(), channel(-1), enabled(false), running(false), startTag(0),
   aboveSafeFillLevel(false), safeFillLevel(100000000), fillLevel(0), filled(0)
{
}

FunctionGenerator::~FunctionGenerator()
{
  busyConnection.disconnect(); // do run cooldownChannel any more
  dev->getDevice().release_irq(irq);
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

void FunctionGenerator::refill()
{
  etherbone::Cycle cycle;
  eb_data_t write_offset_d, read_offset_d;
  
  assert (channel != -1);
  
  eb_address_t regs = shm + FG_REGS_BASE(channel, num_channels);
  
  cycle.open(dev->getDevice());
  cycle.read(regs + FG_WPTR, EB_DATA32, &write_offset_d);
  cycle.read(regs + FG_RPTR, EB_DATA32, &read_offset_d);
  cycle.close();
  
  unsigned write_offset = write_offset_d;
  unsigned read_offset  = read_offset_d;
  
  unsigned remaining = wrapping_sub(write_offset, read_offset, buffer_size);
  unsigned completed = filled - remaining;
  
  for (unsigned i = 0; i < completed; ++i) {
    fillLevel -= fifo.front().duration();
    --filled;
    fifo.pop_front();
  }
  updateAboveSafeFillLevel();
  
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

void FunctionGenerator::irq_handler(eb_data_t status)
{
  // ignore spurious interrupt
  if (channel == -1) return;
  
  // !!! imprecise; should be timestamped by the hardware
  guint64 time = dev->getCurrentTime();
  
  if (status == IRQ_DAT_REFILL) {
    refill();
  } else if (status == IRQ_DAT_START) {
    running = true;
    Started(time);
    enabled = false; // hardware just flipped this itself
    Enabled(enabled);
  } else { // stopped?
    bool hardwareMacroUnderflow = (status != IRQ_DAT_STOP_EMPTY);
    bool microControllerUnderflow = fifo.size() != filled && !hardwareMacroUnderflow;
    executedParameterCount = getExecutedParameterCount();
    running = false;
    fillLevel = 0;
    fifo.clear();
    releaseChannel(true); // true = can be re-used immediately
    Stopped(time, hardwareMacroUnderflow, microControllerUnderflow);
    updateAboveSafeFillLevel();
  }
}

Glib::RefPtr<FunctionGenerator> FunctionGenerator::create(const Glib::ustring& objectPath, ConstructorType args)
{
  return RegisteredObject<FunctionGenerator>::create(objectPath, args);
}

guint64 FunctionGenerator::ParameterTuple::duration() const
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

void FunctionGenerator::AppendParameterSet(
  const std::vector< gint16 >& coeff_a,
  const std::vector< gint16 >& coeff_b,
  const std::vector< gint32 >& coeff_c,
  const std::vector< unsigned char >& step,
  const std::vector< unsigned char >& freq,
  const std::vector< unsigned char >& shift_a,
  const std::vector< unsigned char >& shift_b)
{
  ownerOnly();
  
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
  
  if (channel == -1) {
    updateAboveSafeFillLevel();
  } else {
    refill(); // calls updateAboveSafeFillLevel()
  }
}

void FunctionGenerator::Flush()
{
  ownerOnly();
  
  if (running)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Running, cannot Flush");
  if (enabled)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Enabled, cannot Flush");
    
  assert (channel == -1);
  
  fillLevel = 0;
  fifo.clear();
  updateAboveSafeFillLevel();
}

guint32 FunctionGenerator::getVersion() const
{
  return version;
}

unsigned char FunctionGenerator::getSCUbusSlot() const
{
  return scubusSlot;
}

unsigned char FunctionGenerator::getDeviceNumber() const
{
  return deviceNumber;
}

bool FunctionGenerator::getEnabled() const
{
  return enabled;
}

guint32 FunctionGenerator::getStartTag() const
{
  return startTag;
}

bool FunctionGenerator::getRunning() const
{
  return running;
}

guint64 FunctionGenerator::getFillLevel() const
{
  return fillLevel;
}

guint64 FunctionGenerator::getSafeFillLevel() const
{
  return safeFillLevel;
}

bool FunctionGenerator::getAboveSafeFillLevel() const
{
  return aboveSafeFillLevel;
}

unsigned char FunctionGenerator::getOutputWindowSize() const
{
  return outputWindowSize;
}

guint16 FunctionGenerator::getExecutedParameterCount() const
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

void FunctionGenerator::acquireChannel()
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
  eb_address_t regs = shm + FG_REGS_BASE(channel, num_channels);
  etherbone::Cycle cycle;
  cycle.open(dev->getDevice());
  cycle.write(regs + FG_WPTR,       EB_DATA32, 0);
  cycle.write(regs + FG_RPTR,       EB_DATA32, 0);
  cycle.write(regs + FG_IRQ,        EB_DATA32, irq);
  cycle.write(regs + FG_MACRO_NUM,  EB_DATA32, index);
  cycle.write(regs + FG_RAMP_COUNT, EB_DATA32, 0);
  cycle.write(regs + FG_TAG,        EB_DATA32, startTag);
  cycle.close();
  
  allocation->indexes[i] = index;
  channel = i;
}

bool FunctionGenerator::cooldownChannel(int oldChannel)
{
  busyConnection.disconnect();
  allocation->indexes[oldChannel] = -1;
  return false;
}

void FunctionGenerator::releaseChannel(bool immediate)
{
  assert (channel != -1);
  
  // disable the channel
  dev->getDevice().write(swi + SWI_DISABLE, EB_DATA32, channel);
  
  if (immediate) {
    allocation->indexes[channel] = -1;
  } else {
    // schedule availability for re-use in 10 milliseconds
    // this allows time for the hardware to cleanly stop
    busyConnection = Glib::signal_timeout().connect(
      sigc::bind(sigc::mem_fun(*this, &FunctionGenerator::cooldownChannel), channel),
      10);
  }
  
  channel = -1;
  filled = 0;
}

void FunctionGenerator::setEnabled(bool val)
{
  ownerOnly();
  
  if (val != enabled) {
    if (!val) {
      // there is a race condition here. potentially the tag arrives while we are disabling.
      // this is unavoidable and documented.
      if (!running) releaseChannel(false); // false - go into 10ms cooldown
    } else {
      // check for all the reasons we might not allow enable
      if (fillLevel == 0)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "FillLevel is zero, cannot Enable");
      if (running)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Running, cannot Enable");
      if (busyConnection)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "On 10ms cooldown, cannot Enable");
      
      // actually enable it
      acquireChannel();
      refill();
      dev->getDevice().write(swi + SWI_ENABLE, EB_DATA32, channel);
    }
    enabled = val;
    Enabled(enabled);
  }
}

void FunctionGenerator::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  setEnabled(false);
}

void FunctionGenerator::setStartTag(guint32 val)
{
  ownerOnly();
  if (enabled)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Enabled, cannot setStartTag");
  
  if (val != startTag) {
    startTag = val;
    StartTag(startTag);
  }
}

void FunctionGenerator::setSafeFillLevel(guint64 val)
{
  ownerOnly();
  if (safeFillLevel < 1000000)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "safeFillLevel must be at least 1ms");
  
  if (val != safeFillLevel) {
    safeFillLevel = val;
    SafeFillLevel(safeFillLevel);
    updateAboveSafeFillLevel();
  }
}

void FunctionGenerator::updateAboveSafeFillLevel()
{
  bool val = fillLevel >= safeFillLevel;
  if (val != aboveSafeFillLevel) {
    aboveSafeFillLevel = val;
    AboveSafeFillLevel(aboveSafeFillLevel);
  }
}

}
