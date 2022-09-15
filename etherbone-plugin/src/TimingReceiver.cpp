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


#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>

#include <saftbus/error.hpp>

#include "TimingReceiver.hpp"
#include "SoftwareActionSink.hpp"
#include "SoftwareActionSink_Service.hpp"

#include "eca_regs.h"
#include "eca_flags.h"
#include "eca_queue_regs.h"
#include "fg_regs.h"
#include "ats_regs.h"


namespace eb_plugin {

// Device::irqMap Device::irqs;   // this is in globals.cpp
// Device::msiQueue Device::msis; // this is in globals.cpp

bool TimingReceiver::aquire_watchdog() {
	// try to acquire watchdog
	eb_data_t retry;
	device.read(watchdog, EB_DATA32, &watchdog_value);
	if ((watchdog_value & 0xFFFF) != 0) {
		return false;
	}
	device.write(watchdog, EB_DATA32, watchdog_value);
	device.read(watchdog, EB_DATA32, &retry);
	if (((retry ^ watchdog_value) >> 16) != 0) {
		return false;
	}
	return true;
}

void TimingReceiver::setupGatewareInfo(uint32_t address)
{
	eb_data_t buffer[256];

	etherbone::Cycle cycle;
	cycle.open(device);
	for (unsigned i = 0; i < sizeof(buffer)/sizeof(buffer[0]); ++i) {
		cycle.read(address + i*4, EB_DATA32, &buffer[i]);
	}
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
		gateware_info[key] = value;
	}
}

std::map< std::string, std::string > TimingReceiver::getGatewareInfo() const
{
  return gateware_info;
}

bool TimingReceiver::poll()
{
	std::cerr << "TimingReceiver::poll()" << std::endl;
	getLocked();
	device.write(watchdog, EB_DATA32, watchdog_value);
	return true;
}


TimingReceiver::TimingReceiver(SAFTd *sd, etherbone::Socket &socket, const std::string &obj_path, const std::string &n, const std::string eb_path, saftbus::Container *cont)
	: saftd(sd)
	, object_path(obj_path)
	, name(n)
	, etherbone_path(eb_path)
	, container(cont)
{
	std::cerr << "TimingReceiver::TimingReceiver" << std::endl;
	stat(etherbone_path.c_str(), &dev_stat);
	object_path.append("/");
	object_path.append(name);
	device.open(socket, etherbone_path.c_str());

	// // This just reads the MSI address range out of the ehterbone config space registers
	// // It does not actually enable anything ... MSIs also work without this
	device.enable_msi(&first, &last);

	// Confirm the device is an aligned power of 2
	eb_address_t size = last - first;
	if (((size + 1) & size) != 0) {
		device.close();
		chmod(etherbone_path.c_str(), dev_stat.st_mode);
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has strange sized MSI range");
	}
	if ((first & size) != 0) {
		device.close();
		chmod(etherbone_path.c_str(), dev_stat.st_mode);
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has unaligned MSI first address");
	}


	std::vector<etherbone::sdb_msi_device> ecas_dev, mbx_msi_dev;
	std::vector<sdb_device> streams_dev, infos_dev, watchdogs_dev, scubus_dev, pps_dev, mbx_dev, ats_dev;
	eb_address_t ats_addr = 0; // not every Altera FPGA model has a temperature sensor, i.e, Altera II

	device.sdb_find_by_identity_msi(ECA_SDB_VENDOR_ID, ECA_SDB_DEVICE_ID, ecas_dev);
	device.sdb_find_by_identity_msi(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx_msi_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x8752bf45, streams_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x2d39fa8b, infos_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0xb6232cd3, watchdogs_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x9602eb6f, scubus_dev);
	device.sdb_find_by_identity(0xce42, 0xde0d8ced, pps_dev);
	device.sdb_find_by_identity(ATS_SDB_VENDOR_ID,  ATS_SDB_DEVICE_ID, ats_dev);
	device.sdb_find_by_identity(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx_dev);

	// only support super basic hardware for now
	if (ecas_dev.size() != 1 || streams_dev.size() != 1 || infos_dev.size() != 1 || watchdogs_dev.size() != 1 
		|| pps_dev.size() != 1 || mbx_dev.size() != 1 || mbx_msi_dev.size() != 1) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has insuficient hardware resources");
	}

	base      = ecas_dev[0].sdb_component.addr_first;
    stream    = (eb_address_t)streams_dev[0].sdb_component.addr_first;
    info      = (eb_address_t)infos_dev[0].sdb_component.addr_first;
    watchdog  = (eb_address_t)watchdogs_dev[0].sdb_component.addr_first;
    pps       = (eb_address_t)pps_dev[0].sdb_component.addr_first;

    if (!aquire_watchdog()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Timing Receiver already locked");
    }
	setupGatewareInfo(info);

	// update locked status ...
	poll();
	//    ... and repeat every 1s 
	poll_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&TimingReceiver::poll, this), std::chrono::milliseconds(1000), std::chrono::milliseconds(1000)
		);

	// ECA Setup

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
	// InoutImpl::probe(this, actionSinks, eventSources);

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
			for (; raw_capacity; --raw_capacity) {
				device.write(queue_addresses[i] + ECA_QUEUE_POP_OWR, EB_DATA32, 1);
			}
		}

		for (unsigned num = 0; num < raw_max_num; ++num) {
			switch (raw_type) {
				case ECA_LINUX: {
					// defer construction till demanded by NewSoftwareActionSink, but setup the keys
					actionSinks[SinkKey(i, num)].reset(); // this will create the unique_ptr, but with no content
					// clear any stale valid count
					popMissingQueue(i, num);
					break;
				}
				// case ECA_WBM: {
				// // !!! unsupported
				// break;
				// }
				// case ECA_SCUBUS: {
				// std::vector<sdb_device> scubus;
				// device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x9602eb6f, scubus);
				// if (scubus.size() == 1 && num == 0) {
				// std::string path = getObjectPath() + "/scubus";
				// SCUbusActionSink::ConstructorType args = { path, this, "scubus", i, (eb_address_t)scubus[0].sdb_component.addr_first };
				// actionSinks[SinkKey(i, num)] = SCUbusActionSink::create(args);
				// }
				// break;
				// }
				// case ECA_EMBEDDED_CPU: {
				// #if DEBUG_COMPILE
				// clog << kLogDebug << "ECA: Found queue..." << std::endl;
				// #endif
				// for (unsigned queue_id = 1; queue_id < channels; ++queue_id) {
				// eb_data_t get_id;
				// cycle.open(device);
				// cycle.read(queue_addresses[queue_id]+ECA_QUEUE_QUEUE_ID_GET, EB_DATA32, &get_id);
				// cycle.close();
				// #if DEBUG_COMPILE
				// clog << kLogDebug << "ECA: Found queue @ 0x" << std::hex << queue_addresses[queue_id] << std::dec << std::endl;
				// clog << kLogDebug << "ECA: Found queue with ID: " << get_id << std::endl;
				// #endif
				// if (get_id == ECA_EMBEDDED_CPU) {
				// #if DEBUG_COMPILE
				// clog << kLogDebug << "ECA: Found embedded CPU channel!" << std::endl;
				// #endif
				// std::string path = getObjectPath() + "/embedded_cpu";
				// EmbeddedCPUActionSink::ConstructorType args = { path, this, "embedded_cpu", i, queue_addresses[queue_id] };
				// actionSinks[SinkKey(i, num)] = EmbeddedCPUActionSink::create(args);
				// }
				// }
				// break;
				// }
				default: {
					//clog << kLogWarning << "ECA: unsupported channel type detected" << std::endl;
					// std::cerr << "ECA: unsupported channel type detected" << std::endl;
					break;
				}
			}
		}
	}
}


TimingReceiver::~TimingReceiver() 
{
	std::cerr << "TimingReceiver::~TimingReceiver" << std::endl;
	if (container) {
		for (auto &actionSink: actionSinks) {
			if (actionSink.second) {
				std::cerr << "   remove " << actionSink.second->getObjectPath() << std::endl;
				container->remove_object_delayed(actionSink.second->getObjectPath());

			}
		}
	}

	std::cerr << "saftbus::Loop::get_default().remove(poll_timeout_source)" << std::endl;
	saftbus::Loop::get_default().remove(poll_timeout_source);
	std::cerr << "device.close()" << std::endl;
	device.close();
	std::cerr << "fix device file" << std::endl;
	chmod(etherbone_path.c_str(), dev_stat.st_mode);

}

const std::string &TimingReceiver::get_object_path() const
{
	return object_path;
}

void TimingReceiver::Remove() {
	throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver::Remove is deprecated, use SAFTd::Remove instead");
}
std::string TimingReceiver::getEtherbonePath() const
{
	return etherbone_path;
}
std::string TimingReceiver::getName() const
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


std::string TimingReceiver::getGatewareVersion() const
{
  std::map< std::string, std::string >         gatewareInfo;
  std::map<std::string, std::string>::iterator j;
  std::string                                    rawVersion;
  std::string                                    findString = "-v";
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
		if (SigLocked) {
			SigLocked(locked);
		}
	}

	return newLocked;
}


static inline bool not_isalnum_(char c)
{
  return !(isalnum(c) || c == '_');
}

std::string TimingReceiver::NewSoftwareActionSink(const std::string& name_)
{
	// Is there an available software channel?
	ActionSinks::iterator alloc;
	for (alloc = actionSinks.begin(); alloc != actionSinks.end(); ++alloc) {
		if (!alloc->second) break;
	}
  
	if (alloc == actionSinks.end()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "ECA has no available linux-facing queues");
	}

	std::string seq, name;
	std::ostringstream str;
	str.imbue(std::locale("C"));
	str << "_" << ++sas_count;
	seq = str.str();

	if (name_ == "") {
		name = seq;
	} else {
		name = name_;
		if (name[0] == '_') {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; leading _ is reserved");
		}
		if (find_if(name.begin(), name.end(), not_isalnum_) != name.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
		}

		std::map< std::string, std::string > sinks = getSoftwareActionSinks();
		if (sinks.find(name) != sinks.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Name already in use");
		}
	}
  
  // nest the object under our own name
  std::string path = get_object_path() + "/software/" + seq;
  
  unsigned channel = alloc->first.first;
  unsigned num     = alloc->first.second;
  eb_address_t address = queue_addresses[channel];
  // sigc::slot<void> destroy = sigc::bind(sigc::mem_fun(this, &TimingReceiver::do_remove), alloc->first);
  


	// SoftwareActionSink::ConstructorType args = { path, this, name, channel, num, address, destroy };
	std::unique_ptr<SoftwareActionSink> software_action_sink(new SoftwareActionSink(path, this, name, channel, num, address, container));
	// softwareActionSink->initOwner(getConnection(), getSender());
	if (container) {
		std::unique_ptr<SoftwareActionSink_Service> service(new SoftwareActionSink_Service(software_action_sink.get()));
		container->create_object(path, std::move(service));
	}
	alloc->second = std::move(software_action_sink);

  
	return path;
}

std::map< std::string, std::string > TimingReceiver::getSoftwareActionSinks() const
{
  typedef ActionSinks::const_iterator iterator;
  std::map< std::string, std::string > out;
  for (iterator i = actionSinks.begin(); i != actionSinks.end(); ++i) {
    SoftwareActionSink* softwareActionSink = dynamic_cast<SoftwareActionSink*>(i->second.get());
    if (!softwareActionSink) continue;
    out[softwareActionSink->getObjectName()] = softwareActionSink->getObjectPath();
  }
  return out;
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
  uint64_t  key;    // open?first:last
  bool     open;
  uint64_t  subkey; // open?last:first
  int64_t   offset;
  uint32_t  tag;
  uint8_t   flags;
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
  uint64_t event;
  int16_t  index;
  SearchEntry(uint64_t e, int16_t i) : event(e), index(i) { }
};

struct WalkEntry {
  int16_t   next;
  int64_t   offset;
  uint32_t  tag;
  uint8_t   flags;
  unsigned channel;
  unsigned num;
  WalkEntry(int16_t n, const ECA_OpenClose& oc) : next(n), 
    offset(oc.offset), tag(oc.tag), flags(oc.flags), channel(oc.channel), num(oc.num) { }
};

void TimingReceiver::compile()
{
	std::cerr << "TimingReceiver::compile" << std::endl;
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
      if (oc.subkey != UINT64_MAX) {
        oc.open = false;
        std::swap(oc.key, oc.subkey);
        ++oc.key;
        id_space.push_back(oc);
      }
    }
  }
  
  // Don't proceed if too many actions for the ECA
  if (id_space.size()/2 >= max_conditions)
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Too many active conditions for hardware");
  
  // Sort it by the open/close criteria
  std::sort(id_space.begin(), id_space.end());
  
  // Representation used in hardware
  typedef std::vector<SearchEntry> Search;
  typedef std::vector<WalkEntry> Walk;
  Search search;
  Walk walk;
  int16_t next = -1;
  uint64_t cursor = 0;
  
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
        throw saftbus::Error(saftbus::Error::INVALID_ARGS, "TimingReceiver: Impossible mismatched open/close");
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
    cycle.write(base + ECA_SEARCH_RW_FIRST_RW,    EB_DATA32, (uint16_t)se.index);
    cycle.write(base + ECA_SEARCH_RW_EVENT_HI_RW, EB_DATA32, se.event >> 32);
    cycle.write(base + ECA_SEARCH_RW_EVENT_LO_RW, EB_DATA32, (uint32_t)se.event);
    cycle.write(base + ECA_SEARCH_WRITE_OWR,      EB_DATA32, 1);
    cycle.close();
  }
  
  for (unsigned i = 0; i < walk.size(); ++i) {
    const WalkEntry& we = walk[i];
    
    cycle.open(device);
    cycle.write(base + ECA_WALKER_SELECT_RW,       EB_DATA32, i);
    cycle.write(base + ECA_WALKER_RW_NEXT_RW,      EB_DATA32, (uint16_t)we.next);
    cycle.write(base + ECA_WALKER_RW_OFFSET_HI_RW, EB_DATA32, (uint64_t)we.offset >> 32); // don't sign-extend on shift
    cycle.write(base + ECA_WALKER_RW_OFFSET_LO_RW, EB_DATA32, (uint32_t)we.offset);
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


} // namespace saftlib


