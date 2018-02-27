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
#define DEBUG_COMPILE 0

#include <sstream>
#include <algorithm>
#include <map>

#include "RegisteredObject.h"
#include "Driver.h"
#include "TimingReceiver.h"
#include "SCUbusActionSink.h"
#include "EmbeddedCPUActionSink.h"
#include "SoftwareActionSink.h"
#include "SoftwareCondition.h"
#include "FunctionGenerator.h"
#include "FunctionGeneratorImpl.h"
#include "MasterFunctionGenerator.h"
#include "eca_regs.h"
#include "eca_queue_regs.h"
#include "eca_flags.h"
#include "fg_regs.h"
#include "clog.h"
#include "InoutImpl.h"
#include "Output.h"
#include "Input.h"

namespace saftlib {

TimingReceiver::TimingReceiver(const ConstructorType& args)
 : BaseObject(args.objectPath),
   device(args.device),
   name(args.name),
   etherbonePath(args.etherbonePath),
   
   base(args.base.sdb_component.addr_first),
   stream(args.stream),
   watchdog(args.watchdog),
   pps(args.pps),
   sas_count(0),
   locked(false)
{
  // try to acquire watchdog
  eb_data_t retry;
  device.read(watchdog, EB_DATA32, &watchdog_value);
  if ((watchdog_value & 0xFFFF) != 0)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Timing Receiver already locked");
  device.write(watchdog, EB_DATA32, watchdog_value);
  device.read(watchdog, EB_DATA32, &retry);
  if (((retry ^ watchdog_value) >> 16) != 0)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Timing Receiver already locked");
  
  // parse eb-info data
  setupGatewareInfo(args.info);
  
  // update locked status
  getLocked();

  // poll every 1s some registers
  pollConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &TimingReceiver::poll), 1000);
  
  // Probe the configuration of the ECA
  eb_data_t raw_channels, raw_search, raw_walker, raw_type, raw_max_num, raw_capacity;
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.read(base + ECA_CHANNELS_GET,        EB_DATA32, &raw_channels);
  cycle.read(base + ECA_SEARCH_CAPACITY_GET, EB_DATA32, &raw_search);
  cycle.read(base + ECA_WALKER_CAPACITY_GET, EB_DATA32, &raw_walker);
  cycle.close();
  
  // Initilize members
  channels = raw_channels;
  search_size = raw_search;
  walker_size = raw_walker;
  
  // Worst-case assumption
  max_conditions = std::min(search_size/2, walker_size);
  
  // Prepare local channel state
  queue_addresses.resize(channels, 0);
  most_full.resize(channels, 0);
  
  // remove all old rules
  compile();
  
  // Locate all queue interfaces
  std::vector<sdb_device> queues;
  device.sdb_find_by_identity(ECA_QUEUE_SDB_VENDOR_ID, ECA_QUEUE_SDB_DEVICE_ID, queues);
  
  // Figure out which queues correspond to which channels
  for (unsigned i = 0; i < queues.size(); ++i) {
    eb_data_t id;
    eb_address_t address = queues[i].sdb_component.addr_first;
    device.read(address + ECA_QUEUE_QUEUE_ID_GET, EB_DATA32, &id);
    ++id; // id=0 is the reserved GPIO channel
    if (id >= channels) continue;
    queue_addresses[id] = address;
  }
  
  // Create the IOs (channel 0)
  InoutImpl::probe(this, actionSinks, eventSources);
  
  // Configure the non-IO action sinks, creating objects and clearing status
  for (unsigned i = 1; i < channels; ++i) {
    cycle.open(device);
    cycle.write(base + ECA_CHANNEL_SELECT_RW,    EB_DATA32, i);
    cycle.read (base + ECA_CHANNEL_TYPE_GET,     EB_DATA32, &raw_type);
    cycle.read (base + ECA_CHANNEL_MAX_NUM_GET,  EB_DATA32, &raw_max_num);
    cycle.read (base + ECA_CHANNEL_CAPACITY_GET, EB_DATA32, &raw_capacity);
    cycle.close();
    
    // Flush any queue we manage
    if (raw_type == ECA_LINUX) {
      for (; raw_capacity; --raw_capacity)
        device.write(queue_addresses[i] + ECA_QUEUE_POP_OWR, EB_DATA32, 1);
    }
    
    for (unsigned num = 0; num < raw_max_num; ++num) {
      switch (raw_type) {
        case ECA_LINUX: {
          // defer construction till demanded by NewSoftwareActionSink, but setup the keys
          actionSinks[SinkKey(i, num)].reset();
          // clear any stale valid count
          popMissingQueue(i, num);
          break;
        }
        case ECA_WBM: {
          // !!! unsupported
          break;
        }
        case ECA_SCUBUS: {
          std::vector<sdb_device> scubus;
          device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x9602eb6f, scubus);
          if (scubus.size() == 1 && num == 0) {
            Glib::ustring path = getObjectPath() + "/scubus";
            SCUbusActionSink::ConstructorType args = { path, this, "scubus", i, (eb_address_t)scubus[0].sdb_component.addr_first };
            actionSinks[SinkKey(i, num)] = SCUbusActionSink::create(args);
          }
          break;
        }
        case ECA_EMBEDDED_CPU: {
#if DEBUG_COMPILE
            clog << kLogDebug << "ECA: Found queue..." << std::endl;
#endif
            for (unsigned queue_id = 1; queue_id < channels; ++queue_id) {
              eb_data_t get_id;
              cycle.open(device);
              cycle.read(queue_addresses[queue_id]+ECA_QUEUE_QUEUE_ID_GET, EB_DATA32, &get_id);
              cycle.close();
#if DEBUG_COMPILE
              clog << kLogDebug << "ECA: Found queue @ 0x" << std::hex << queue_addresses[queue_id] << std::dec << std::endl;
              clog << kLogDebug << "ECA: Found queue with ID: " << get_id << std::endl;
#endif
              if (get_id == ECA_EMBEDDED_CPU) {
#if DEBUG_COMPILE
                clog << kLogDebug << "ECA: Found embedded CPU channel!" << std::endl;
#endif
                Glib::ustring path = getObjectPath() + "/embedded_cpu";
                EmbeddedCPUActionSink::ConstructorType args = { path, this, "embedded_cpu", i, queue_addresses[queue_id] };
                actionSinks[SinkKey(i, num)] = EmbeddedCPUActionSink::create(args);
              }
            }
          break;
        }
        default: {
          clog << kLogWarning << "ECA: unsupported channel type detected" << std::endl;
          break;
        }
      }
    }
  }
  
  // hook all MSIs
  for (unsigned i = 0; i < channels; ++i) {
    // Reserve an MSI
    channel_msis.push_back(device.request_irq(
      args.base,
      sigc::bind(sigc::mem_fun(*this, &TimingReceiver::msiHandler), i)));
    // Hook MSI to hardware
    setHandler(i, true, channel_msis.back());
    // Wipe out old global state for the channel => will generate an MSI => updateMostFull
    resetMostFull(i);
  }
}

TimingReceiver::~TimingReceiver()
{
  try { // destructors may not throw
    pollConnection.disconnect();
    
    // Disable all MSIs
    for (unsigned i = 0; i < channels; ++i) {
      setHandler(i, false, 0);
      device.release_irq(channel_msis[i]);
    }
    
    // destroy children
    actionSinks.clear();
    eventSources.clear();
    otherStuff.clear();
    
    // wipe out the condition table
    compile();
    
  } catch (const etherbone::exception_t& e) {
    clog << kLogErr << "TimingReceiver::~TimingReceiver: " << e << std::endl;
  } catch (const Glib::Error& e) {
    clog << kLogErr << "TimingReceiver::~TimingReceiver: " << e.what() << std::endl;
  } catch (...) {
    clog << kLogErr << "TimingReceiver::~TimingReceiver: unknown exception" << std::endl;
  }
}

void TimingReceiver::setHandler(unsigned channel, bool enable, eb_address_t address)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
  cycle.write(base + ECA_CHANNEL_MSI_SET_ENABLE_OWR, EB_DATA32, 0);
  cycle.write(base + ECA_CHANNEL_MSI_SET_TARGET_OWR, EB_DATA32, address);
  cycle.write(base + ECA_CHANNEL_MSI_SET_ENABLE_OWR, EB_DATA32, enable?1:0);
  cycle.close();
  clog << kLogDebug << "TimingReceiver: registered irq 0x" << std::hex << address << std::endl;
}

bool TimingReceiver::poll()
{
  getLocked();
  device.write(watchdog, EB_DATA32, watchdog_value);
  return true;
}

void TimingReceiver::Remove()
{
  SAFTd::get().RemoveDevice(name);
}

Glib::ustring TimingReceiver::getEtherbonePath() const
{
  return etherbonePath;
}

Glib::ustring TimingReceiver::getName() const
{
  return name;
}

#define WR_API_VERSION          4

#if WR_API_VERSION <= 3
#define WR_PPS_GEN_ESCR_MASK    0x6       //bit 1: PPS valid, bit 2: TS valid
#else
#define WR_PPS_GEN_ESCR_MASK    0xc       //bit 2: PPS valid, bit 3: TS valid
#endif
#define WR_PPS_GEN_ESCR         0x1c      //External Sync Control Register

void TimingReceiver::setupGatewareInfo(guint32 address)
{
  eb_data_t buffer[256];
  
  etherbone::Cycle cycle;
  cycle.open(device);
  for (unsigned i = 0; i < sizeof(buffer)/sizeof(buffer[0]); ++i)
    cycle.read(address + i*4, EB_DATA32, &buffer[i]);
  cycle.close();
  
  std::string str;
  for (unsigned i = 0; i < sizeof(buffer)/sizeof(buffer[0]); ++i) {
    str.push_back((buffer[i] >> 24) & 0xff);
    str.push_back((buffer[i] >> 16) & 0xff);
    str.push_back((buffer[i] >>  8) & 0xff);
    str.push_back((buffer[i] >>  0) & 0xff);
  }
  
  std::stringstream ss(str);
  std::string line;
  while (std::getline(ss, line))
  {
    if (line.empty() || line[0] == ' ') continue; // skip empty/history lines
    
    std::string::size_type offset = line.find(':');
    if (offset == std::string::npos) continue; // not a field
    if (offset+2 >= line.size()) continue;
    std::string value(line, offset+2);
    
    std::string::size_type tail = line.find_last_not_of(' ', offset-1);
    if (tail == std::string::npos) continue;
    std::string key(line, 0, tail+1);
    
    // store the field
    info[key] = value;
  }
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getGatewareInfo() const
{
  return info;
}

Glib::ustring TimingReceiver::getGatewareVersion() const
{
  std::map< Glib::ustring, Glib::ustring >         gatewareInfo;
  std::map<Glib::ustring, Glib::ustring>::iterator j;
  Glib::ustring                                    rawVersion;
  Glib::ustring                                    findString = "-v";
  int                                              pos = 0;

  gatewareInfo = getGatewareInfo();
  j = gatewareInfo.begin();        // build date
  j++;                             // gateware version
  rawVersion = j->second;
  pos = rawVersion.find(findString, 0);

  if ((pos <= 0) || (((pos + findString.length()) >= rawVersion.length()))) return ("N/A");

  pos = pos + findString.length(); // get rid of findString '-v'
  
  return(rawVersion.substr(pos, rawVersion.length() - pos));
}

bool TimingReceiver::getLocked() const
{
  eb_data_t data;
  device.read(pps + WR_PPS_GEN_ESCR, EB_DATA32, &data);
  bool newLocked = (data & WR_PPS_GEN_ESCR_MASK) == WR_PPS_GEN_ESCR_MASK;
  
  /* Update signal */
  if (newLocked != locked) {
    locked = newLocked;
    Locked(locked);
  }
  
  return newLocked;
}

void TimingReceiver::do_remove(SinkKey key)
{
  actionSinks[key].reset();
  SoftwareActionSinks(getSoftwareActionSinks());
  Interfaces(getInterfaces());
  compile();
}

static inline bool not_isalnum_(char c)
{
  return !(isalnum(c) || c == '_');
}

Glib::ustring TimingReceiver::NewSoftwareActionSink(const Glib::ustring& name_)
{
  // Is there an available software channel?
  ActionSinks::iterator alloc;
  for (alloc = actionSinks.begin(); alloc != actionSinks.end(); ++alloc)
    if (!alloc->second) break;
  
  if (alloc == actionSinks.end())
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "ECA has no available linux-facing queues");
  
  Glib::ustring seq, name;
  std::ostringstream str;
  str.imbue(std::locale("C"));
  str << "_" << ++sas_count;
  seq = str.str();
  
  if (name_ == "") {
    name = seq;
  } else {
    name = name_;
    if (name[0] == '_')
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; leading _ is reserved");
    if (find_if(name.begin(), name.end(), not_isalnum_) != name.end())
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
    
    std::map< Glib::ustring, Glib::ustring > sinks = getSoftwareActionSinks();
    if (sinks.find(name) != sinks.end())
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Name already in use");
  }
  
  // nest the object under our own name
  Glib::ustring path = getObjectPath() + "/software/" + seq;
  
  unsigned channel = alloc->first.first;
  unsigned num     = alloc->first.second;
  eb_address_t address = queue_addresses[channel];
  sigc::slot<void> destroy = sigc::bind(sigc::mem_fun(this, &TimingReceiver::do_remove), alloc->first);
  
  SoftwareActionSink::ConstructorType args = { path, this, name, channel, num, address, destroy };
  Glib::RefPtr<ActionSink> softwareActionSink = SoftwareActionSink::create(args);
  softwareActionSink->initOwner(getConnection(), getSender());
  
  // Own it and inform changes to properties
  alloc->second = softwareActionSink;
  SoftwareActionSinks(getSoftwareActionSinks());
  Interfaces(getInterfaces());
  
  return path;
}

void TimingReceiver::InjectEvent(guint64 event, guint64 param, guint64 time)
{
  etherbone::Cycle cycle;
  
  cycle.open(device);
  cycle.write(stream, EB_DATA32, event >> 32);
  cycle.write(stream, EB_DATA32, event & 0xFFFFFFFFUL);
  cycle.write(stream, EB_DATA32, param >> 32);
  cycle.write(stream, EB_DATA32, param & 0xFFFFFFFFUL);
  cycle.write(stream, EB_DATA32, 0); // reserved
  cycle.write(stream, EB_DATA32, 0); // TEF
  cycle.write(stream, EB_DATA32, time >> 32);
  cycle.write(stream, EB_DATA32, time & 0xFFFFFFFFUL);
  cycle.close();
}

guint64 TimingReceiver::ReadRawCurrentTime()
{
  etherbone::Cycle cycle;
  eb_data_t time1, time0, time2;
  
  do {
    cycle.open(device);
    cycle.read(base + ECA_TIME_HI_GET, EB_DATA32, &time1);
    cycle.read(base + ECA_TIME_LO_GET, EB_DATA32, &time0);
    cycle.read(base + ECA_TIME_HI_GET, EB_DATA32, &time2);
    cycle.close();
  } while (time1 != time2);
  
  return guint64(time1) << 32 | time0;
}

guint64 TimingReceiver::ReadCurrentTime()
{
  if (!locked)
    throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, "TimingReceiver is not Locked");

  return ReadRawCurrentTime();
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getSoftwareActionSinks() const
{
  typedef ActionSinks::const_iterator iterator;
  std::map< Glib::ustring, Glib::ustring > out;
  for (iterator i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    Glib::RefPtr<SoftwareActionSink> softwareActionSink =
      Glib::RefPtr<SoftwareActionSink>::cast_dynamic(i->second);
    if (!softwareActionSink) continue;
    out[softwareActionSink->getObjectName()] = softwareActionSink->getObjectPath();
  }
  return out;
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getOutputs() const
{
  typedef ActionSinks::const_iterator iterator;
  std::map< Glib::ustring, Glib::ustring > out;
  for (iterator i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    Glib::RefPtr<Output> output =
      Glib::RefPtr<Output>::cast_dynamic(i->second);
    if (!output) continue;
    out[output->getObjectName()] = output->getObjectPath();
  }
  return out;
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInputs() const
{
  typedef EventSources::const_iterator iterator;
  std::map< Glib::ustring, Glib::ustring > out;
  for (iterator i = eventSources.begin(); i != eventSources.end(); ++i) {
    Glib::RefPtr<Input> input =
      Glib::RefPtr<Input>::cast_dynamic(i->second);
    if (!input) continue;
    out[input->getObjectName()] = input->getObjectPath();
  }
  return out;
}

std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > TimingReceiver::getInterfaces() const
{
  std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > out;
  
  typedef ActionSinks::const_iterator sink;
  for (sink i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    if (!i->second) continue; // skip inactive software action sinks
    out[i->second->getInterfaceName()][i->second->getObjectName()] = i->second->getObjectPath();
  }
  
  typedef EventSources::const_iterator source;
  for (source i = eventSources.begin(); i != eventSources.end(); ++i) {
    out[i->second->getInterfaceName()][i->second->getObjectName()] = i->second->getObjectPath();
  }
  
  typedef OtherStuff::const_iterator other;
  typedef Owneds::const_iterator owned;
  for (other i = otherStuff.begin(); i != otherStuff.end(); ++i)
    for (owned j = i->second.begin(); j != i->second.end(); ++j)
      out[i->first][j->first] = j->second->getObjectPath();
  
  return out;
}

guint32 TimingReceiver::getFree() const
{
  return max_conditions - used_conditions;
}

void TimingReceiver::msiHandler(eb_data_t msi, unsigned channel)
{
  unsigned code = msi >> 16;
  unsigned num  = msi & 0xFFFF;
  
  // clog << kLogDebug << "MSI: " << channel << " " << num << " " << code << std::endl;
  
  // MAX_FULL is tracked by this object, not the subchannel
  if (code == ECA_MAX_FULL) {
    updateMostFull(channel);
  } else {
    SinkKey k(channel, num);
    ActionSinks::iterator i = actionSinks.find(k);
    if (i == actionSinks.end()) {
      clog << kLogErr << "MSI for non-existant channel " << channel << " num " << num << std::endl;
    } else if (!i->second) {
      // It could be that the user deleted the SoftwareActionSink
      // while it still had pending actions. Pop any stale records.
      if (code == ECA_VALID) popMissingQueue(channel, num);
    } else {
      i->second->receiveMSI(code);
    }
  }
}

guint16 TimingReceiver::updateMostFull(unsigned channel)
{
  if (channel >= most_full.size()) return 0;
  
  etherbone::Cycle cycle;
  eb_data_t raw;
  
  cycle.open(device);
  cycle.write(base + ECA_CHANNEL_SELECT_RW,        EB_DATA32, channel);
  cycle.read (base + ECA_CHANNEL_MOSTFULL_ACK_GET, EB_DATA32, &raw);
  cycle.close();
  
  guint16 mostFull = raw & 0xFFFF;
  guint16 used     = raw >> 16;
  
  if (most_full[channel] != mostFull) {
    most_full[channel] = mostFull;
    
    // Broadcast new value to all subchannels
    SinkKey low(channel, 0);
    SinkKey high(channel+1, 0);
    ActionSinks::iterator first = actionSinks.lower_bound(low);
    ActionSinks::iterator last  = actionSinks.lower_bound(high);
    
    for (; first != last; ++first) {
      if (!first->second) continue; // skip unused SoftwareActionSinks
      first->second->MostFull(mostFull);
    }
  }
  
  return used;
}

void TimingReceiver::resetMostFull(unsigned channel)
{
  if (channel >= most_full.size()) return;
  
  etherbone::Cycle cycle;
  eb_data_t null;
  
  cycle.open(device);
  cycle.write(base + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
  cycle.read (base + ECA_CHANNEL_MOSTFULL_CLEAR_GET, EB_DATA32, &null);
  cycle.close();
}

void TimingReceiver::popMissingQueue(unsigned channel, unsigned num)
{
  etherbone::Cycle cycle;
  eb_data_t nill;
  
  // First, rearm the MSI
  cycle.open(device);
  device.write(base + ECA_CHANNEL_SELECT_RW,       EB_DATA32, channel);
  device.write(base + ECA_CHANNEL_NUM_SELECT_RW,   EB_DATA32, num);
  device.read (base + ECA_CHANNEL_VALID_COUNT_GET, EB_DATA32, &nill);
  cycle.close();
  // Then pop the ignored record
  device.write(queue_addresses[channel] + ECA_QUEUE_POP_OWR, EB_DATA32, 1);
}

struct ECA_OpenClose {
  guint64  key;    // open?first:last
  bool     open;
  guint64  subkey; // open?last:first
  gint64   offset;
  guint32  tag;
  guint8   flags;
  unsigned channel;
  unsigned num;
};

// Using this heuristic, perfect containment never duplicates walk records
static bool operator < (const ECA_OpenClose& a, const ECA_OpenClose& b)
{
  if (a.key < b.key) return true;
  if (a.key > b.key) return false;
  if (!a.open && b.open) return true; // close first
  if (a.open && !b.open) return false;
  if (a.subkey > b.subkey) return true; // open largest first, close smallest first
  if (a.subkey < b.subkey) return false;
  // order does not matter (popping does not depend on content)
  return false;
}

struct SearchEntry {
  guint64 event;
  gint16  index;
  SearchEntry(guint64 e, gint16 i) : event(e), index(i) { }
};

struct WalkEntry {
  gint16   next;
  gint64   offset;
  guint32  tag;
  guint8   flags;
  unsigned channel;
  unsigned num;
  WalkEntry(gint16 n, const ECA_OpenClose& oc) : next(n), 
    offset(oc.offset), tag(oc.tag), flags(oc.flags), channel(oc.channel), num(oc.num) { }
};

void TimingReceiver::compile()
{
  // Store all active conditions into a vector for processing
  typedef std::vector<ECA_OpenClose> ID_Space;
  ID_Space id_space;

  // Step one is to find all active conditions on all action sinks
  for (ActionSinks::const_iterator sink = actionSinks.begin(); sink != actionSinks.end(); ++sink) {
    if (!sink->second) continue; // skip unassigned software action sinks
    for (ActionSink::Conditions::const_iterator condition = sink->second->getConditions().begin(); condition != sink->second->getConditions().end(); ++condition) {
      if (!condition->second->getActive()) continue;
      
      // Memorize the condition
      ECA_OpenClose oc;
      oc.key     = condition->second->getID() &  condition->second->getMask();
      oc.open    = true;
      oc.subkey  = condition->second->getID() | ~condition->second->getMask();
      oc.offset  = condition->second->getOffset();
      oc.tag     = condition->second->getRawTag();
      oc.flags   =(condition->second->getAcceptLate()    ?(1<<ECA_LATE)    :0) |
                  (condition->second->getAcceptEarly()   ?(1<<ECA_EARLY)   :0) |
                  (condition->second->getAcceptConflict()?(1<<ECA_CONFLICT):0) |
                  (condition->second->getAcceptDelayed() ?(1<<ECA_DELAYED) :0);
      oc.channel = sink->second->getChannel();
      oc.num     = sink->second->getNum();
      
      // Push the open record
      id_space.push_back(oc);
      
      // Push the close record (if any)
      if (oc.subkey != G_MAXUINT64) {
        oc.open = false;
        std::swap(oc.key, oc.subkey);
        ++oc.key;
        id_space.push_back(oc);
      }
    }
  }
  
  // Don't proceed if too many actions for the ECA
  if (id_space.size()/2 >= max_conditions)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Too many active conditions for hardware");
  
  // Sort it by the open/close criteria
  std::sort(id_space.begin(), id_space.end());
  
  // Representation used in hardware
  typedef std::vector<SearchEntry> Search;
  typedef std::vector<WalkEntry> Walk;
  Search search;
  Walk walk;
  gint16 next = -1;
  guint64 cursor = 0;
  
  // Special-case at zero: always push a leading record
  if (id_space.empty() || id_space[0].key != 0)
    search.push_back(SearchEntry(0, next));
  
  // Walk the remaining records and transform them to hardware!
  unsigned i = 0;
  while (i < id_space.size()) {
    cursor = id_space[i].key;
    
    // pop the walker stack for all closes
    while (i < id_space.size() && cursor == id_space[i].key && !id_space[i].open) {
      if (next == -1)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "TimingReceiver: Impossible mismatched open/close");
      next = walk[next].next;
      ++i;
    }
    
    // push the opens
    while (i < id_space.size() && cursor == id_space[i].key && id_space[i].open) {
      walk.push_back(WalkEntry(next, id_space[i]));
      next = walk.size()-1;
      ++i;
    }
    
    search.push_back(SearchEntry(cursor, next));
  }
  
#if DEBUG_COMPILE
  clog << kLogDebug << "Table compilation complete!" << std::endl;
  for (i = 0; i < search.size(); ++i)
    clog << kLogDebug << "S: " << search[i].event << " " << search[i].index << std::endl;
  for (i = 0; i < walk.size(); ++i)
    clog << kLogDebug << "W: " << walk[i].next << " " << walk[i].offset << " " << walk[i].tag << " " << walk[i].flags << " " << (int)walk[i].channel << " " << (int)walk[i].num << std::endl;
#endif

  etherbone::Cycle cycle;
  for (unsigned i = 0; i < search_size; ++i) {
    /* Duplicate last entry to fill out the table */
    const SearchEntry& se = (i<search.size())?search[i]:search.back();
    
    cycle.open(device);
    cycle.write(base + ECA_SEARCH_SELECT_RW,      EB_DATA32, i);
    cycle.write(base + ECA_SEARCH_RW_FIRST_RW,    EB_DATA32, (guint16)se.index);
    cycle.write(base + ECA_SEARCH_RW_EVENT_HI_RW, EB_DATA32, se.event >> 32);
    cycle.write(base + ECA_SEARCH_RW_EVENT_LO_RW, EB_DATA32, (guint32)se.event);
    cycle.write(base + ECA_SEARCH_WRITE_OWR,      EB_DATA32, 1);
    cycle.close();
  }
  
  for (unsigned i = 0; i < walk.size(); ++i) {
    const WalkEntry& we = walk[i];
    
    cycle.open(device);
    cycle.write(base + ECA_WALKER_SELECT_RW,       EB_DATA32, i);
    cycle.write(base + ECA_WALKER_RW_NEXT_RW,      EB_DATA32, (guint16)we.next);
    cycle.write(base + ECA_WALKER_RW_OFFSET_HI_RW, EB_DATA32, (guint64)we.offset >> 32); // don't sign-extend on shift
    cycle.write(base + ECA_WALKER_RW_OFFSET_LO_RW, EB_DATA32, (guint32)we.offset);
    cycle.write(base + ECA_WALKER_RW_TAG_RW,       EB_DATA32, we.tag);
    cycle.write(base + ECA_WALKER_RW_FLAGS_RW,     EB_DATA32, we.flags);
    cycle.write(base + ECA_WALKER_RW_CHANNEL_RW,   EB_DATA32, we.channel);
    cycle.write(base + ECA_WALKER_RW_NUM_RW,       EB_DATA32, we.num);
    cycle.write(base + ECA_WALKER_WRITE_OWR,       EB_DATA32, 1);
    cycle.close();
  }
  
  // Flip the tables
  device.write(base + ECA_FLIP_ACTIVE_OWR, EB_DATA32, 1);
  
  used_conditions = id_space.size()/2;
}

void TimingReceiver::probe(OpenDevice& od)
{
  std::vector<etherbone::sdb_msi_device> ecas, mbx_msi;
  od.device.sdb_find_by_identity_msi(ECA_SDB_VENDOR_ID, ECA_SDB_DEVICE_ID, ecas);
  od.device.sdb_find_by_identity_msi(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx_msi);
  
  std::vector<sdb_device> streams, infos, watchdogs, scubus, pps, mbx;
  od.device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x8752bf45, streams);
  od.device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x2d39fa8b, infos);
  od.device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0xb6232cd3, watchdogs);
  od.device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x9602eb6f, scubus);
  od.device.sdb_find_by_identity(0xce42, 0xde0d8ced, pps);
  od.device.sdb_find_by_identity(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx);
  
  // only support super basic hardware for now
  if (ecas.size() != 1 || streams.size() != 1 || infos.size() != 1 || watchdogs.size() != 1 
	|| pps.size() != 1 || mbx.size() != 1 || mbx_msi.size() != 1)
    return;
 
 
  TimingReceiver::ConstructorType args = { 
    od.device, 
    od.name,
    od.etherbonePath, 
    od.objectPath,
    ecas[0],
    (eb_address_t)streams[0].sdb_component.addr_first,
    (eb_address_t)infos[0].sdb_component.addr_first,
    (eb_address_t)watchdogs[0].sdb_component.addr_first,
    (eb_address_t)pps[0].sdb_component.addr_first,
  };
  Glib::RefPtr<TimingReceiver> tr = RegisteredObject<TimingReceiver>::create(od.objectPath, args);
  od.ref = tr;
    
  // Add special SCU hardware
  if (scubus.size() == 1) {
    etherbone::Cycle cycle;
    
    // Probe for LM32 block memories
    eb_address_t fgb = 0;
    std::vector<sdb_device> fgs, rom;
    od.device.sdb_find_by_identity(LM32_RAM_USER_VENDOR,    LM32_RAM_USER_PRODUCT,    fgs);
    od.device.sdb_find_by_identity(LM32_CLUSTER_ROM_VENDOR, LM32_CLUSTER_ROM_PRODUCT, rom);
    
    
    if (rom.size() != 1)
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "SCU is missing LM32 cluster ROM");
    
    eb_address_t rom_address = rom[0].sdb_component.addr_first;
    eb_data_t cpus, eps_per;
    cycle.open(od.device);
    cycle.read(rom_address + 0x0, EB_DATA32, &cpus);
    cycle.read(rom_address + 0x4, EB_DATA32, &eps_per);
    cycle.close();
    
    if (cpus != fgs.size())
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Number of LM32 RAMs does not equal ROM cpu_count");
    
    // Check them all for the function generator microcontroller
    unsigned i;
    for (i = 0; i < fgs.size(); ++i) {
      eb_data_t magic, version;
      fgb = fgs[i].sdb_component.addr_first;
      
      cycle.open(od.device);
      cycle.read(fgb + SHM_BASE + FG_MAGIC_NUMBER, EB_DATA32, &magic);
      cycle.read(fgb + SHM_BASE + FG_VERSION,      EB_DATA32, &version);
      cycle.close();
      if (magic == 0xdeadbeef && version == 3) break;
    }
    
    // Did we find a function generator?
    if (i != fgs.size()) {
	
      // get mailbox slot number for swi host=>lm32
      eb_data_t mb_slot;
      cycle.open(od.device);
      cycle.read(fgb + SHM_BASE + FG_MB_SLOT, EB_DATA32, &mb_slot);
      cycle.close();	

      if (mb_slot < 0 && mb_slot > 127)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "mailbox slot number not in range 0 to 127");

      // swi address of fg is to be found in mailbox slot mb_slot
      eb_address_t swi = mbx[0].sdb_component.addr_first + mb_slot * 4 * 2;
      clog << kLogDebug << "mailbox address for swi is 0x" << std::hex << swi << std::endl;
      eb_data_t num_channels, buffer_size, macros[FG_MACROS_SIZE];
      
      // Probe the configuration and hardware macros
      cycle.open(od.device);
      cycle.read(fgb + SHM_BASE + FG_NUM_CHANNELS, EB_DATA32, &num_channels);
      cycle.read(fgb + SHM_BASE + FG_BUFFER_SIZE,  EB_DATA32, &buffer_size);
      for (unsigned j = 0; j < FG_MACROS_SIZE; ++j)
        cycle.read(fgb + SHM_BASE + FG_MACROS + j*4, EB_DATA32, &macros[j]);
      cycle.close();
      
      // Create an allocation buffer
      Glib::RefPtr<FunctionGeneratorChannelAllocation> allocation(
        new FunctionGeneratorChannelAllocation);
      allocation->indexes.resize(num_channels, -1);
      
      // Disable all channels
      cycle.open(od.device);
      for (unsigned j = 0; j < num_channels; ++j)
        cycle.write(swi, EB_DATA32, SWI_DISABLE | j);
      cycle.close();

//			std::vector<Glib::RefPtr<FunctionGenerator>> functionGenerators;      
			std::vector<std::shared_ptr<FunctionGeneratorImpl>> functionGeneratorImplementations;      			
      // Create the objects to control the channels
      for (unsigned j = 0; j < FG_MACROS_SIZE; ++j) {
              if (!macros[j]) continue; // no hardware


        
        std::ostringstream spath;
        spath.imbue(std::locale("C"));
        spath << od.objectPath << "/fg_" << j;
        Glib::ustring path = spath.str();
        
        FunctionGeneratorImpl::ConstructorType args = { path, tr.operator->(), allocation, fgb, swi, mbx_msi[0], mbx[0], (unsigned)num_channels, (unsigned)buffer_size, j, (guint32)macros[j] };


        std::shared_ptr<FunctionGeneratorImpl> fgImpl(new FunctionGeneratorImpl(args));
				functionGeneratorImplementations.push_back(fgImpl);
				
				FunctionGenerator::ConstructorType fgargs = { path, tr.operator->(), fgImpl };
        Glib::RefPtr<FunctionGenerator> fg = FunctionGenerator::create(fgargs);				
        std::ostringstream name;
        name.imbue(std::locale("C"));
        name << "fg-" << (int)fgImpl->getSCUbusSlot() << "-" << (int)fgImpl->getDeviceNumber();
        tr->otherStuff["FunctionGenerator"][name.str()] = fg;

      }
      
      // Create a master object to control all channels

//      give references to existing fg objects
//      provide single d-bus interface to all fg objects
      std::ostringstream spath;
      spath.imbue(std::locale("C"));
      spath << od.objectPath << "/masterfg";
      Glib::ustring path = spath.str();

      MasterFunctionGenerator::ConstructorType args = { path, tr.operator->(), functionGeneratorImplementations };
      Glib::RefPtr<MasterFunctionGenerator> fg = MasterFunctionGenerator::create(args);

      tr->otherStuff["MasterFunctionGenerator"]["masterfg"] = fg;
    }
  }
}

static Driver<TimingReceiver> timingReceiver;

}
