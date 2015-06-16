#define ETHERBONE_THROWS 1
//#define DEBUG_COMPRESS 1
//#define DEBUG_COMPILE 1

#include <sstream>
#include <iostream>
#include <algorithm>

#include "RegisteredObject.h"
#include "Driver.h"
#include "TimingReceiver.h"
#include "SoftwareCondition.h"
#include "eca_regs.h"

namespace saftlib {

TimingReceiver::TimingReceiver(ConstructorType args)
 : device(args.device),
   name(args.name),
   etherbonePath(args.etherbonePath),
   base(args.base),
   stream(args.stream),
   queue(args.queue),
   sas_count(0),
   overflow_irq(device.request_irq(sigc::mem_fun(*this, &TimingReceiver::overflow_handler))),
   arrival_irq (device.request_irq(sigc::mem_fun(*this, &TimingReceiver::arrival_handler)))
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
  
  //channels = (sizes >> 8) & 0xFF;
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
  
  // Enable aq channel
  cycle.open(device);
  // select
  cycle.write(base + ECAC_SELECT, EB_DATA32, aq_channel << 16);
  // enable (clear drain and freeze flags)
  cycle.write(base + ECAC_CTL, EB_DATA32, (ECAC_CTL_DRAIN|ECAC_CTL_FREEZE)<<8);
  cycle.close();
  
  // enable interrupts and actions
  device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_DISABLE<<8 | ECA_CTL_INT_ENABLE);
}

TimingReceiver::~TimingReceiver()
{
  try { // destructors may not throw
    device.release_irq(overflow_irq);
    device.release_irq(arrival_irq);
    setHandlers(false);
    
    // disable aq channel
    etherbone::Cycle cycle;
    cycle.open(device);
    cycle.write(base + ECAC_SELECT, EB_DATA32, aq_channel << 16);
    cycle.write(base + ECAC_CTL, EB_DATA32, (ECAC_CTL_DRAIN|ECAC_CTL_FREEZE));
    cycle.close();
    
    // Disable interrupts and the ECA
    device.write(base + ECA_CTL, EB_DATA32, ECA_CTL_INT_ENABLE<<8 | ECA_CTL_DISABLE);
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA::~ECA: " << e << std::endl;
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

void TimingReceiver::Remove()
{
  SAFTd::get()->RemoveDevice(name);
}

Glib::ustring TimingReceiver::getEtherbonePath() const
{
  return etherbonePath;
}

Glib::ustring TimingReceiver::getName() const
{
  return name;
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

guint64 TimingReceiver::getCurrentTime() const
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
  typedef std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator iterator;
  for (iterator i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    out[i->second->getInterfaceName()][i->first] = i->second->getObjectPath();
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
  try { // interrupt handlers may not throw
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
    param = event1; param <<= 32; param |= param0;
    time  = time1;  time  <<= 32; time  |= time0;
    
    bool conflict = (flags & 2) != 0;
    bool late     = (flags & 2) != 0;
    
    for (std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator sink = actionSinks.begin(); sink != actionSinks.end(); ++sink) {
      bool match = false;
      for (std::list< Glib::RefPtr<Condition> >::const_iterator condition = sink->second->getConditions().begin(); condition != sink->second->getConditions().end(); ++condition) {
        Glib::RefPtr<SoftwareCondition> softwareCondition =
          Glib::RefPtr<SoftwareCondition>::cast_dynamic(*condition);
        if (!softwareCondition) continue;
        
        if (((softwareCondition->getID() ^ event) & softwareCondition->getMask()) == 0 &&
            softwareCondition->getOffset()/8 == tag2delay[tag]) { // !!! remove /8 with new hardware
          softwareCondition->Action(event, param, time*8, -1, late, false, conflict); // !!! remove *8 with new hardware
          match = true;
        }
      }
      // !!! if (match && conflict) sink->second->sawConflict();
      // !!! if (match && late)     sink->second->sawLate();
    }
  } catch (const etherbone::exception_t& e) {
    std::cerr << "ECA::arrival_handler: " << e << std::endl;
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
  typedef std::map<guint64, int> Offsets;
  typedef std::vector<ECA_Merge> Merges;
  Offsets offsets;
  Merges merges;
  guint32 next_tag = 0;
  
  // Step one is to merge overlapping, but compatible, ranges
  for (std::map< Glib::ustring, Glib::RefPtr<ActionSink> >::const_iterator sink = actionSinks.begin(); sink != actionSinks.end(); ++sink) {
    for (std::list< Glib::RefPtr<Condition> >::const_iterator condition = sink->second->getConditions().begin(); condition != sink->second->getConditions().end(); ++condition) {
      guint64 first  = (*condition)->getID() & (*condition)->getMask();
      guint64 last   = (*condition)->getID() | ~(*condition)->getMask();
      guint64 offset = (*condition)->getOffset()/8; // !!! remove /8 with new hardware
      gint32  channel= sink->second->getChannel();
      guint32 tag    = (*condition)->getRawTag();
      
      // reject idiocy
      if (first > last)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "first must be <= last");
      
      // software tag is based on offset
      if (Glib::RefPtr<SoftwareCondition>::cast_dynamic(*condition)) {
        std::pair<Offsets::iterator,bool> result = 
          offsets.insert(std::pair<guint64, guint32>(offset, next_tag));
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
  
  unsigned i = 0, j;
  while (i < merges.size()) {
    // Merge overlapping/touching records
    while ((j=i+1) < merges.size() && 
           merges[i].channel == merges[j].channel &&
           merges[i].offset  == merges[j].offset  &&
           (merges[j].first   == 0                ||
            merges[i].last    >= merges[j].first-1)) {
#if DEBUG_COMPRESS
      std::cerr << "I: " << merges[i].first << " " << merges[i].last << " " << merges[i].offset << " " << merges[i].channel << " " << merges[i].tag << std::endl;
      std::cerr << "I: " << merges[j].first << " " << merges[j].last << " " << merges[j].offset << " " << merges[j].channel << " " << merges[j].tag << std::endl;
#endif
      // they overlap, so tags must match!
      if (merges[i].tag != merges[j].tag)
        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Conflicting tags for overlapping conditions (same channel, same offset, same event)");
      // merge!
      merges[j].first = merges[i].first;
      merges[j].last  = std::max(merges[j].last, merges[i].last);
      i = j;
    }
    // push combined record to open/close pass
#if DEBUG_COMPRESS
    std::cerr << "O: " << merges[i].first << " " << merges[i].last << " " << merges[i].offset << " " << merges[i].channel << " " << merges[i].tag << std::endl;
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
  std::cerr << "Table compilation complete! Reflows necessary: " << reflows << "\n";
  for (i = 0; i < search.size(); ++i)
    std::cerr << "S: " << search[i].event << " " << search[i].index << "\n";
  for (i = 0; i < walk.size(); ++i)
    std::cerr << "W: " << walk[i].offset << " " << walk[i].tag << " " << walk[i].next << " " << (int)walk[i].channel << "\n";
  std::cerr << std::flush; 
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
    cycle.write(base + ECA_DELAY0,  EB_DATA32, we.offset & 0xFFFFFFFFUL);
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
  // !!! probe for real
  TimingReceiver::ConstructorType args = { od.device, od.name, od.etherbonePath, 0x80, 0x7ffffff0, 0x40 };
  od.ref = RegisteredObject<TimingReceiver>::create(od.objectPath, args);
}

static Driver<TimingReceiver> timingReceiver;

}
