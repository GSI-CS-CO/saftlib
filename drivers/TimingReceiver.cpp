#define ETHERBONE_THROWS 1
//#define DEBUG_COMPRESS 1
//#define DEBUG_COMPILE 1

#include <sstream>
#include <algorithm>

#include "RegisteredObject.h"
#include "Driver.h"
#include "TimingReceiver.h"
#include "SCUbusActionSink.h"
#include "SoftwareActionSink.h"
#include "SoftwareCondition.h"
#include "FunctionGenerator.h"
#include "eca_regs.h"
#include "fg_regs.h"
#include "clog.h"

namespace saftlib {

TimingReceiver::TimingReceiver(ConstructorType args)
 : device(args.device),
   name(args.name),
   etherbonePath(args.etherbonePath),
   base(args.base),
   stream(args.stream),
   queue(args.queue),
   pps(args.pps),
   sas_count(0),
   overflow_irq(device.request_irq(sigc::mem_fun(*this, &TimingReceiver::overflow_handler))),
   arrival_irq (device.request_irq(sigc::mem_fun(*this, &TimingReceiver::arrival_handler))),
   locked(false)
{
  eb_data_t name[64];
  eb_data_t sizes, queued, id;
  etherbone::Cycle cycle;
  
  cycle.open(device);
  // probe
  for (unsigned i = 0; i < 64; ++i)
    cycle.read(base + ECA_CTL, EB_DATA32, &name[i]);
  cycle.read(base + ECA_INFO, EB_DATA32, &sizes);
  // disable ECA actions and interrupts
  cycle.write(base + ECA_CTL, EB_DATA32, ECA_CTL_INT_ENABLE<<8 | ECA_CTL_DISABLE);
  cycle.close();
  
  channels = (sizes >> 8) & 0xFF;
  table_size = 1 << ((sizes >> 24) & 0xFF);
  queue_size = 1 << ((sizes >> 16) & 0xFF);
  
  // remove all old rules
  compile();
  
  cycle.open(device);
  // Clear old counters
  cycle.write(queue + ECAQ_DROPPED, EB_DATA32, 0);
  // How much old Q junk to flush?
  cycle.read(queue + ECAQ_QUEUED, EB_DATA32, &queued);
  // Which channel does this sit on?
  cycle.read(queue + ECAQ_META, EB_DATA32, &id);
  cycle.close();
  
  aq_channel = (id >> 16) & 0xFF;
  
  // drain the Q
  unsigned pop = 0;
  while (pop < queued) {
    unsigned batch = ((queued-pop)>64)?64:(queued-pop); // pop 64 at once
    cycle.open(device);
    for (unsigned i = 0; i < batch; ++i)
      cycle.write(queue + ECAQ_CTL, EB_DATA32, 1);
    cycle.close();
    pop += batch;
  }
  
  // Hook interrupts
  setHandlers(true, arrival_irq, overflow_irq);
  
  // Enable all channels
  cycle.open(device);
  for (unsigned int channel = 0; channel < channels; ++channel) {
    cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
    cycle.write(base + ECAC_CTL, EB_DATA32, (ECAC_CTL_DRAIN|ECAC_CTL_FREEZE)<<8);
  }
  cycle.close();
  
  // enable interrupts and actions
  device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_DISABLE<<8 | ECA_CTL_INT_ENABLE);
  
  // update locked status
  getLocked();

  // poll every 1s some registers
  pollConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &TimingReceiver::poll), 1000);
}

TimingReceiver::~TimingReceiver()
{
  try { // destructors may not throw
    pollConnection.disconnect();
    
    // destroy children before unhooking irqs/etc
    actionSinks.clear();
    generators.clear();
    
    device.release_irq(overflow_irq);
    device.release_irq(arrival_irq);
    setHandlers(false);
    
    // disable aq channel
    etherbone::Cycle cycle;
    cycle.open(device);
    for (unsigned int channel = 0; channel < channels; ++channel) {
      cycle.write(base + ECAC_SELECT, EB_DATA32, channel << 16);
      cycle.write(base + ECAC_CTL, EB_DATA32, (ECAC_CTL_DRAIN|ECAC_CTL_FREEZE));
    }
    cycle.close();
    
    // Disable interrupts and the ECA
    device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_INT_ENABLE<<8 | ECA_CTL_DISABLE);
  } catch (const etherbone::exception_t& e) {
    clog << kLogErr << "ECA::~ECA: " << e << std::endl;
  }
}

void TimingReceiver::setHandlers(bool enable, eb_address_t arrival, eb_address_t overflow)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(queue + ECAQ_INT_MASK, EB_DATA32, 0);
  cycle.write(queue + ECAQ_ARRIVAL,  EB_DATA32, arrival);
  cycle.write(queue + ECAQ_OVERFLOW, EB_DATA32, overflow);
  if (enable) cycle.write(queue + ECAQ_INT_MASK, EB_DATA32, 3);
  cycle.close();
}

bool TimingReceiver::poll()
{
  getLocked();
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

#define WR_PPS_GEN_ESCR         0x1c        //External Sync Control Register
#define WR_PPS_GEN_ESCR_MASK    0x6         //bit 1: PPS valid, bit 2: TS valid

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

void TimingReceiver::do_remove(Glib::ustring name)
{
  actionSinks.erase(name);
  SoftwareActionSinks(getSoftwareActionSinks());
  Interfaces(getInterfaces());
}

static inline bool not_isalnum_(char c)
{
  return !(isalnum(c) || c == '_');
}

Glib::ustring TimingReceiver::NewSoftwareActionSink(const Glib::ustring& name_)
{
  Glib::ustring name(name_);
  if (name == "") {
    std::ostringstream str;
    str.imbue(std::locale("C"));
    str << "_" << ++sas_count;
    name = str.str();
  } else {
    if (actionSinks.find(name) != actionSinks.end())
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Name already in use");
    if (name[0] == '_')
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; leading _ is reserved");
    if (find_if(name.begin(), name.end(), not_isalnum_) != name.end())
      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
  }
  
  // nest the object under our own name
  Glib::ustring path = getObjectPath() + "/" + name;
  
  sigc::slot<void> destroy = sigc::bind(sigc::mem_fun(this, &TimingReceiver::do_remove), name);
  
  SoftwareActionSink::ConstructorType args = { this, aq_channel, destroy };
  Glib::RefPtr<ActionSink> softwareActionSink = SoftwareActionSink::create(path, args);
  softwareActionSink->initOwner(getConnection(), getSender());
  
  actionSinks[name] = softwareActionSink;
  SoftwareActionSinks(getSoftwareActionSinks());
  Interfaces(getInterfaces());
  
  return path;
}

void TimingReceiver::InjectEvent(guint64 event, guint64 param, guint64 time)
{
  etherbone::Cycle cycle;
  
  time /= 8; // !!! remove with new hardware
  
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
  eb_data_t time1, time0;
  cycle.open(device);
  cycle.read(base + ECA_TIME1, EB_DATA32, &time1);
  cycle.read(base + ECA_TIME0, EB_DATA32, &time0);
  cycle.close();
  
  guint64 result;
  result = time1;
  result <<= 32;
  result |= time0;
  return result*8; // !!! remove *8 with new hardware
}

guint64 TimingReceiver::ReadCurrentTime()
{
  if (!locked)
    throw Gio::DBus::Error(Gio::DBus::Error::IO_ERROR, "TimingReceiver is not Locked");

  return ReadRawCurrentTime();
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getSoftwareActionSinks() const
{
  typedef std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator iterator;
  std::map< Glib::ustring, Glib::ustring > out;
  for (iterator i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    Glib::RefPtr<SoftwareActionSink> softwareActionSink =
      Glib::RefPtr<SoftwareActionSink>::cast_dynamic(i->second);
    if (!softwareActionSink) continue;
    out[i->first] = softwareActionSink->getObjectPath();
  }
  return out;
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getOutputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::map< Glib::ustring, Glib::ustring > TimingReceiver::getInoutputs() const
{
  std::map< Glib::ustring, Glib::ustring > out;
  return out; // !!! not for cryring
}

std::vector< Glib::ustring > TimingReceiver::getGuards() const
{
  std::vector< Glib::ustring > out;
  return out; // !!! not for cryring
}

std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > TimingReceiver::getInterfaces() const
{
  std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > out;
  
  typedef std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator sink;
  for (sink i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    out[i->second->getInterfaceName()][i->first] = i->second->getObjectPath();
  }
  
  typedef std::map< Glib::ustring, Glib::RefPtr<FunctionGenerator> >::const_iterator gen;
  for (gen i = generators.begin(); i != generators.end(); ++i) {
    out["FunctionGenerator"][i->first] = i->second->getObjectPath();
  }
  
  return out;
}

guint32 TimingReceiver::getFree() const
{
  return table_size; // !!!
}

void TimingReceiver::overflow_handler(eb_data_t)
{
  for (std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator sink = actionSinks.begin(); sink != actionSinks.end(); ++sink) {
    // !!! sink->Overflow();
  }
}

void TimingReceiver::arrival_handler(eb_data_t)
{
  etherbone::Cycle cycle;
  eb_data_t flags;
  eb_data_t event1, event0;
  eb_data_t param1, param0;
  eb_data_t tag,    tef;
  eb_data_t time1,  time0;
  
  cycle.open(device);
  cycle.read (queue + ECAQ_FLAGS,  EB_DATA32, &flags);
  cycle.read (queue + ECAQ_EVENT1, EB_DATA32, &event1);
  cycle.read (queue + ECAQ_EVENT0, EB_DATA32, &event0);
  cycle.read (queue + ECAQ_PARAM1, EB_DATA32, &param1);
  cycle.read (queue + ECAQ_PARAM0, EB_DATA32, &param0);
  cycle.read (queue + ECAQ_TAG,    EB_DATA32, &tag);
  cycle.read (queue + ECAQ_TEF,    EB_DATA32, &tef);
  cycle.read (queue + ECAQ_TIME1,  EB_DATA32, &time1);
  cycle.read (queue + ECAQ_TIME0,  EB_DATA32, &time0);
  cycle.write(queue + ECAQ_CTL,    EB_DATA32, 1); // pop
  cycle.close();
  
  guint64 event, param, time;
  event = event1; event <<= 32; event |= event0;
  param = param1; param <<= 32; param |= param0;
  time  = time1;  time  <<= 32; time  |= time0;
  
  // bool conflict = (flags & 2) != 0;
  bool late     = (flags & 1) != 0;
  // !!! delayed ?
  
  for (std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator sink = actionSinks.begin(); sink != actionSinks.end(); ++sink) {
    Glib::RefPtr<SoftwareActionSink> softwareActionSink =
      Glib::RefPtr<SoftwareActionSink>::cast_dynamic(sink->second);
    if (softwareActionSink)
      softwareActionSink->emit(event, param, time*8, -1, tag2delay[tag], late, false); // !!! remove *8 with new hardware
  }
}

struct ECA_Merge {
  gint32  channel;
  gint64  offset;
  guint64 first;
  guint64 last;
  guint32 tag;
  ECA_Merge(gint32 c, gint64 o, guint64 f, guint64 l, guint32 t)
   : channel(c), offset(o), first(f), last(l), tag(t) { }
};

bool operator < (const ECA_Merge& a, const ECA_Merge& b)
{
  if (a.channel < b.channel) return true;
  if (a.channel > b.channel) return false;
  if (a.offset < b.offset) return true;
  if (a.offset > b.offset) return false;
  if (a.first < b.first) return true;
  if (a.first > b.first) return false;
  return false;
}

struct ECA_OpenClose {
  guint64 key;
  bool    open;
  guint64 subkey;
  gint64  offset;
  gint32  channel;
  guint32 tag;
  ECA_OpenClose(guint64 k, bool x, guint64 s, guint64 o, gint32 c, guint32 t)
   : key(k), open(x), subkey(s), offset(o), channel(c), tag(t) { }
};

// Using this heuristic, perfect containment never duplicates walk records
bool operator < (const ECA_OpenClose& a, const ECA_OpenClose& b)
{
  if (a.key < b.key) return true;
  if (a.key > b.key) return false;
  if (!a.open && b.open) return true; // close first
  if (a.open && !b.open) return false;
  if (a.subkey > b.subkey) return true; // open largest first, close smallest last
  if (a.subkey < b.subkey) return false;
  if (a.offset < b.offset) return a.open; // open smallest first, close largest last
  if (a.offset > b.offset) return !a.open;
  if (a.channel < b.channel) return a.open;
  if (a.channel > b.channel) return !a.open;
  if (a.tag < b.tag) return a.open;
  if (a.tag > b.tag) return !a.open;
  return false;
}

struct SearchEntry {
  guint64 event;
  gint16  index;
  SearchEntry(guint64 e, gint16 i) : event(e), index(i) { }
};

struct WalkEntry {
  gint64  offset;
  guint32 tag;
  gint16  next;
  guint8  channel;
  WalkEntry(gint64 o, guint32 t, gint16 n, guint8 c) : offset(o), tag(t), next(n), channel(c) { }
};

void TimingReceiver::compile()
{
  typedef std::map<gint64, int> Offsets;
  typedef std::vector<ECA_Merge> Merges;
  Offsets offsets;
  Merges merges;
  guint32 next_tag = 0;
  
  // Step one is to merge overlapping, but compatible, ranges
  for (std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator sink = actionSinks.begin(); sink != actionSinks.end(); ++sink) {
    for (std::list< Glib::RefPtr<Condition> >::const_iterator condition = sink->second->getConditions().begin(); condition != sink->second->getConditions().end(); ++condition) {
      guint64 first  = (*condition)->getID() & (*condition)->getMask();
      guint64 last   = (*condition)->getID() | ~(*condition)->getMask();
      gint64  offset = (*condition)->getOffset();
      gint32  channel= sink->second->getChannel();
      guint32 tag    = (*condition)->getRawTag();
      
      // reject idiocy
      if (first > last)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "first must be <= last");
      
      // software tag is based on offset
      if (Glib::RefPtr<SoftwareCondition>::cast_dynamic(*condition)) {
        std::pair<Offsets::iterator,bool> result = 
          offsets.insert(std::pair<gint64, guint32>(offset, next_tag));
        guint32 aq_tag = result.first->second;
        if (result.second) ++next_tag; // tag now used
        
        merges.push_back(ECA_Merge(aq_channel, offset, first, last, aq_tag));
      } else {
        merges.push_back(ECA_Merge(channel, offset, first, last, tag));
      }
    }
  }
  
  // Sort it by the merge criteria
  std::sort(merges.begin(), merges.end());
  
  // Compress the conditions: merging overlaps and convert to id-space
  typedef std::vector<ECA_OpenClose> ID_Space;
  ID_Space id_space;

#if DEBUG_COMPRESS || DEBUG_COMPILE
  clog << std::dec;
#endif

  unsigned i = 0, j;
  while (i < merges.size()) {
    // Merge overlapping/touching records
    while ((j=i+1) < merges.size() && 
           merges[i].channel == merges[j].channel &&
           merges[i].offset  == merges[j].offset  &&
           (merges[j].first   == 0                ||
            merges[i].last    >= merges[j].first-1)) {
#if DEBUG_COMPRESS
      clog << kLogDebug << "I: " << merges[i].first << " " << merges[i].last << " " << merges[i].offset << " " << merges[i].channel << " " << merges[i].tag << std::endl;
      clog << kLogDebug << "I: " << merges[j].first << " " << merges[j].last << " " << merges[j].offset << " " << merges[j].channel << " " << merges[j].tag << std::endl;
#endif
      // they overlap, so tags must match!
      if (merges[i].tag != merges[j].tag) {
        if (merges[i].last == merges[j].first-1) { 
          break;
        } else {
          throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Conflicting tags for overlapping conditions (same channel, same offset, same event)");
        }
      }
      // merge!
      merges[j].first = merges[i].first;
      merges[j].last  = std::max(merges[j].last, merges[i].last);
      i = j;
    }
    // push combined record to open/close pass
#if DEBUG_COMPRESS
    clog << kLogDebug << "O: " << merges[i].first << " " << merges[i].last << " " << merges[i].offset << " " << merges[i].channel << " " << merges[i].tag << std::endl;
#endif
    id_space.push_back(ECA_OpenClose(merges[i].first,  true,  merges[i].last,  merges[i].offset, merges[i].channel, merges[i].tag));
    if (merges[i].last != G_MAXUINT64)
      id_space.push_back(ECA_OpenClose(merges[i].last+1, false, merges[i].first, merges[i].offset, merges[i].channel, merges[i].tag));
    i = j;
  }
  
  // Don't need this any more
  merges.clear();
  
  // Sort it by the open/close criteria
  std::sort(id_space.begin(), id_space.end());
  
  // Representation used in hardware
  typedef std::vector<SearchEntry> Search;
  typedef std::vector<WalkEntry> Walk;
  Search search;
  Walk walk;
  Walk reflow;
  gint16 next = -1;
  guint64 cursor = 0;
  int reflows = 0;
  
  // Special-case at zero: skip closes and push leading record
  if (id_space.empty() || id_space[0].key != 0)
    search.push_back(SearchEntry(0, next));
  
  // Walk the remaining records and transform them to hardware!
  i = 0;
  while (i < id_space.size()) {
    cursor = id_space[i].key;
    
    while (i < id_space.size() && cursor == id_space[i].key && !id_space[i].open) {
      while (walk[next].offset  != id_space[i].offset ||
             walk[next].tag     != id_space[i].tag    ||
             walk[next].channel != id_space[i].channel) {
        reflow.push_back(walk[next]);
        next = walk[next].next;
        if (next == -1)
          throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Wes promised this was impossible");
      }
      next = walk[next].next;
      ++i;
    }
    
    // restore reflow records
    for (j = reflow.size(); j > 0; --j) {
      walk.push_back(reflow[j-1]);
      walk.back().next = next;
      next = walk.size()-1;
      ++reflows;
    }
    reflow.clear();
    
    // push the opens
    while (i < id_space.size() && cursor == id_space[i].key && id_space[i].open) {
      // ... could try to find an existing tail to re-use here
      walk.push_back(WalkEntry(id_space[i].offset, id_space[i].tag, next, id_space[i].channel));
      next = walk.size()-1;
      ++i;
    }
    
    search.push_back(SearchEntry(cursor, next));
  }
  
#if DEBUG_COMPILE
  clog << kLogDebug << "Table compilation complete! Reflows necessary: " << reflows << "\n";
  for (i = 0; i < search.size(); ++i)
    clog << kLogDebug << "S: " << search[i].event << " " << search[i].index << "\n";
  for (i = 0; i < walk.size(); ++i)
    clog << kLogDebug << "W: " << walk[i].offset << " " << walk[i].tag << " " << walk[i].next << " " << (int)walk[i].channel << "\n";
  clog << kLogDebug << std::flush; 
#endif

  if (walk.size() > table_size || search.size() > table_size*2)
    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Too many conditions to fit in hardware");
  
  etherbone::Cycle cycle;
  for (unsigned i = 0; i < 2*table_size; ++i) {
    /* Duplicate last entry to fill out the table */
    const SearchEntry& se = (i<search.size())?search[i]:search.back();
    eb_data_t first = (se.index==-1)?0:(0x80000000UL|se.index);
    
    cycle.open(device);
    cycle.write(base + ECA_SEARCH, EB_DATA32, i);
    cycle.write(base + ECA_FIRST,  EB_DATA32, first);
    cycle.write(base + ECA_EVENT1, EB_DATA32, se.event >> 32);
    cycle.write(base + ECA_EVENT0, EB_DATA32, se.event & 0xFFFFFFFFUL);
    cycle.close();
  }
  
  for (unsigned i = 0; i < walk.size(); ++i) {
    const WalkEntry& we = walk[i];
    eb_data_t next = (we.next==-1)?0:(0x80000000UL|we.next);
    
    cycle.open(device);
    cycle.write(base + ECA_WALK,    EB_DATA32, i);
    cycle.write(base + ECA_NEXT,    EB_DATA32, next);
    cycle.write(base + ECA_DELAY1,  EB_DATA32, we.offset >> 32);
    cycle.write(base + ECA_DELAY0,  EB_DATA32, (we.offset/8) & 0xFFFFFFFFUL); // !!! new hardware removes /8
    cycle.write(base + ECA_TAG,     EB_DATA32, we.tag);
    cycle.write(base + ECA_CHANNEL, EB_DATA32, we.channel);
    cycle.close();
  }
  
  // Flip the tables
  device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_FLIP);
  
  // !!! fuck - not atomic WRT table flip
  tag2delay.resize(next_tag);
  for (Offsets::iterator o = offsets.begin(); o != offsets.end(); ++o)
    tag2delay[o->second] = o->first;
}

void TimingReceiver::probe(OpenDevice& od)
{
  // !!! check board ID
  etherbone::Cycle cycle;
  
  std::vector<sdb_device> ecas, queues, streams, scubus, pps;
  od.device.sdb_find_by_identity(GSI_VENDOR_ID, ECA_DEVICE_ID,  ecas);
  od.device.sdb_find_by_identity(GSI_VENDOR_ID, ECAQ_DEVICE_ID, queues);
  od.device.sdb_find_by_identity(GSI_VENDOR_ID, ECAE_DEVICE_ID, streams);
  od.device.sdb_find_by_identity(GSI_VENDOR_ID, 0x9602eb6f, scubus);
  od.device.sdb_find_by_identity(0xce42, 0xde0d8ced, pps);
  
  // only support super basic hardware for now
  if (ecas.size() != 1 || queues.size() != 1 || streams.size() != 1 || pps.size() != 1)
    return;
  
  TimingReceiver::ConstructorType args = { 
    od.device, 
    od.name, 
    od.etherbonePath, 
    (eb_address_t)ecas[0].sdb_component.addr_first,
    (eb_address_t)streams[0].sdb_component.addr_first,
    (eb_address_t)queues[0].sdb_component.addr_first,
    (eb_address_t)pps[0].sdb_component.addr_first,
  };
  Glib::RefPtr<TimingReceiver> tr = RegisteredObject<TimingReceiver>::create(od.objectPath, args);
  od.ref = tr;
  
  // Add special SCU hardware
  if (scubus.size() == 1) {
    // !!! hard-coded to #3
    SCUbusActionSink::ConstructorType args = { tr.operator->(), 3, (eb_address_t)scubus[0].sdb_component.addr_first };
    Glib::ustring path = od.objectPath + "/scubus";
    Glib::RefPtr<ActionSink> actionSink = SCUbusActionSink::create(path, args);
    tr->actionSinks["scubus"] = actionSink;
    
    // Probe for LM32 block memories
    eb_address_t fgb = 0;
    std::vector<sdb_device> fgs, eps, rom;
    od.device.sdb_find_by_identity(LM32_RAM_USER_VENDOR,    LM32_RAM_USER_PRODUCT,    fgs);
    od.device.sdb_find_by_identity(LM32_IRQ_EP_VENDOR,      LM32_IRQ_EP_PRODUCT,      eps);
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
//  currently EPs appear twice on crossbar. consult Mathias.
//    if (eps_per * cpus != eps.size())
//      throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Number of LM32 EPs does not equal ROM cpu_count*ep_count");
    
    // Check them all for the function generator microcontroller
    unsigned i;
    for (i = 0; i < fgs.size(); ++i) {
      eb_data_t magic, version;
      fgb = fgs[i].sdb_component.addr_first;
      
      cycle.open(od.device);
      cycle.read(fgb + SHM_BASE + FG_MAGIC_NUMBER, EB_DATA32, &magic);
      cycle.read(fgb + SHM_BASE + FG_VERSION,      EB_DATA32, &version);
      cycle.close();
      if (magic == 0xdeadbeef && version == 2) break;
    }
    
    // Did we find a function generator?
    if (i != fgs.size()) {
      // SWI is always the second (+1th) IRQ endpoint on the microcontroller
      eb_address_t swi = eps[i*eps_per + 1].sdb_component.addr_first;
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
        cycle.write(swi + SWI_DISABLE, EB_DATA32, j);
      cycle.close();
      
      // Create the objects to control the channels
      for (unsigned j = 0; j < FG_MACROS_SIZE; ++j) {
        if (!macros[j]) continue; // no hardware
        
        std::ostringstream path;
        path.imbue(std::locale("C"));
        path << od.objectPath << "/fg_" << j;
        
        FunctionGenerator::ConstructorType args = { tr.operator->(), allocation, fgb, swi, (unsigned)num_channels, (unsigned)buffer_size, j, (guint32)macros[j] };
        Glib::RefPtr<FunctionGenerator> fg = FunctionGenerator::create(Glib::ustring(path.str()), args);

        std::ostringstream name;
        name << "fg-" << (int)fg->getSCUbusSlot() << "-" << (int)fg->getDeviceNumber();
        
        tr->generators[name.str()] = fg;
      }
    }
  }
}

static Driver<TimingReceiver> timingReceiver;

}
