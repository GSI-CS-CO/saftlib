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

#include "SAFTd.hpp"
#include "TimingReceiver.hpp"
#include "SoftwareActionSink.hpp"
#include "SoftwareActionSink_Service.hpp"

#include "eca_regs.h"
#include "eca_flags.h"
#include "eca_queue_regs.h"
#include "fg_regs.h"
#include "ats_regs.h"


namespace eb_plugin {

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

class WatchdogDriver {
	etherbone::Device &device;
	eb_address_t watchdog;
	eb_data_t watchdog_value;
public:
	WatchdogDriver(etherbone::Device &device);
	bool aquire();
	void update();
};

WatchdogDriver::WatchdogDriver(etherbone::Device &dev) 
	: device(dev) 
{
	std::vector<sdb_device> watchdogs_dev;
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0xb6232cd3, watchdogs_dev);
	if (watchdogs_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no watchdog device found on hardware");
	}
	if (watchdogs_dev.size() > 1) {
		std::cerr << "more than one watchdog device found on hardware, taking the first one" << std::endl;
	}
	watchdog = static_cast<eb_address_t>(watchdogs_dev[0].sdb_component.addr_first);
	if (!aquire()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Watchdog on Timing Receiver already locked");
	}
}
bool WatchdogDriver::aquire() {
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
void WatchdogDriver::update() {
	device.write(watchdog, EB_DATA32, watchdog_value);
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

#define WR_API_VERSION          4

#if WR_API_VERSION <= 3
#define WR_PPS_GEN_ESCR_MASK    0x6       //bit 1: PPS valid, bit 2: TS valid
#else
#define WR_PPS_GEN_ESCR_MASK    0xc       //bit 2: PPS valid, bit 3: TS valid
#endif
#define WR_PPS_GEN_ESCR         0x1c      //External Sync Control Register

#define WR_PPS_VENDOR_ID        0xce42
#define WR_PPS_DEVICE_ID        0xde0d8ced

class PpsDriver {
	friend class TimingReceiver;
	etherbone::Device &device;
	std::function<void(bool locked)> &SigLocked;
	eb_address_t pps;
	mutable bool locked;
public:
	PpsDriver(etherbone::Device &dev, std::function<void(bool locked)> &sl);
	bool getLocked() const;
};

PpsDriver::PpsDriver(etherbone::Device &dev, std::function<void(bool locked)> &sl)
	: device(dev)
	, SigLocked(sl)
{
	std::vector<sdb_device> pps_dev;
	device.sdb_find_by_identity(WR_PPS_VENDOR_ID, WR_PPS_DEVICE_ID, pps_dev);
	if (pps_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no WR-PPS device found on hardware");
	}
	if (pps_dev.size() > 1) {
		std::cerr << "more than one WR-PPS device found on hardware, taking the first one" << std::endl;
	}
	pps = static_cast<eb_address_t>(pps_dev[0].sdb_component.addr_first);
	getLocked();
}

bool PpsDriver::getLocked() const
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

	// just to see if msis work: use MBOX to send us a telegram
	// device.write(mbox_for_testing_only + 4, EB_DATA32, 0x10f00); // configure MSI
	// device.write(mbox_for_testing_only + 0, EB_DATA32, 0xaffe);  // send MSI

	return newLocked;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

#define EVENT_SDB_DEVICE_ID             0x8752bf45


EcaDriver::EcaDriver(SAFTd *sd, etherbone::Device &dev, const std::string &obj_path, saftbus::Container *cont)
	: saftd(sd)
	, device(dev)
	, object_path(obj_path)
	, container(cont)
	, sas_count(0)
{
	std::vector<etherbone::sdb_msi_device> ecas_dev;
	std::vector<sdb_device> streams_dev;

	device.sdb_find_by_identity_msi(ECA_SDB_VENDOR_ID, ECA_SDB_DEVICE_ID, ecas_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, EVENT_SDB_DEVICE_ID, streams_dev);

	std::cerr << "ecas.msi_first=" << std::hex << std::setw(8) << std::setfill('0') << ecas_dev[0].msi_first 
	          << "     msi_last="  << std::hex << std::setw(8) << std::setfill('0') << ecas_dev[0].msi_last
	          << std::dec
	          << std::endl;

	if (ecas_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no ECA_UNIT:CONTROL device found on hardware");
	}
	if (ecas_dev.size() > 1) {
		std::cerr << "more than one ECA_UNIT:CONTROL devices found on hardware, taking the first one" << std::endl;
	}

	if (streams_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no ECA_UNIT:EVENTS_IN device found on hardware");
	}
	if (streams_dev.size() > 1) {
		std::cerr << "more than one ECA_UNIT:EVENTS_IN devices found on hardware, taking the first one" << std::endl;
	}

	base      = ecas_dev[0].sdb_component.addr_first;
    stream    = (eb_address_t)streams_dev[0].sdb_component.addr_first;

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

	ECAchannels.resize(channels);

	std::cerr << "TimingReceiver with " << channels << " ECA channels" << std::endl;

	// Create the IOs (channel 0)
	// InoutImpl::probe(this, actionSinks, eventSources);

	// Configure the non-IO action sinks, creating objects and clearing status
	// for (unsigned i = 1; i < channels; ++i) {
	ECA_LINUX_channel = nullptr; // initialize with null, if a ECA_LINUX_channel is found this will point to it
	ECA_LINUX_channel_index = -1;
	for (unsigned channel_idx = 1; channel_idx < channels; ++channel_idx) {
		cycle.open(device);
		cycle.write(base + ECA_CHANNEL_SELECT_RW,    EB_DATA32, channel_idx);
		cycle.read (base + ECA_CHANNEL_TYPE_GET,     EB_DATA32, &raw_type);
		cycle.read (base + ECA_CHANNEL_MAX_NUM_GET,  EB_DATA32, &raw_max_num);
		cycle.read (base + ECA_CHANNEL_CAPACITY_GET, EB_DATA32, &raw_capacity);
		cycle.close();

		std::cerr << "channel=" << channel_idx << "   raw_max_num=" << raw_max_num <<	std::endl;

		switch(raw_type) {
			case ECA_LINUX:
				// Flush any queue we manage
				for (unsigned i = 0; i < raw_capacity; ++i) {
					device.write(queue_addresses[channel_idx] + ECA_QUEUE_POP_OWR, EB_DATA32, 1);
				}
				// clear any stale valid count
				for (unsigned num = 0; num < raw_max_num; ++num) {
					popMissingQueue(channel_idx, num);
				}
				// Take a pointer the the first Linux facing queue
				if (ECA_LINUX_channel == nullptr) {
					ECA_LINUX_channel_index = channel_idx;
					ECA_LINUX_channel = &ECAchannels[channel_idx];
					ECA_LINUX_channel_subchannels = raw_max_num;
					ECA_LINUX_channel->reserve(raw_max_num); // allocate enough memory for all hareware resources
				} else {
					std::cerr << "more than one Linux facing ECA channel. We will use the first of them and ignore the rest" << std::endl;
				}
			break;
			// case ECA_WBM:
			// break;
			// case ECA_SCUBUS:
			// break;
			// case ECA_EMBEDDED_CPU:
			// break;
		}
	}


	// This just reads the MSI address range out of the ehterbone config space registers
	// It does not actually enable anything ... MSIs also work without this
	device.enable_msi(&first, &last);
	std::cerr << "TimingReceiver enable_msi first=0x" << std::hex << std::setw(8) << std::setfill('0') << first 
	          <<                          " last=0x"  << std::hex << std::setw(8) << std::setfill('0') << last << std::endl;

	// Confirm the device is an aligned power of 2
	eb_address_t size = last - first;
	if (((size + 1) & size) != 0) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has strange sized MSI range");
	}
	if ((first & size) != 0) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has unaligned MSI first address");
	}

	// set MSI handlers
	etherbone::sdb_msi_device& sdb = ecas_dev[0];
	for (unsigned channel_idx = 0; channel_idx < channels; ++channel_idx) {

		// Confirm we had SDB records for MSI all the way down
		if (sdb.msi_last < sdb.msi_first) {
			throw etherbone::exception_t("request_irq/non_msi_crossbar_inbetween", EB_FAIL);
		}

		// Confirm that first is aligned to size
		// e.g. first = 0x10000  last = 0x1ffff 
		// => size_mask = 0x0ffff
		// => 0x10000 & 0x0ffff = 0
		eb_address_t size_mask = sdb.msi_last - sdb.msi_first;
		// msi_first and msi_last is the address range in which the msi_master (on hardware) can reach the host
		// for a host connected with PCIe, this might be msi_first=0x10000 and msi_last=0x1ffff
		// for a host connected with USB (same hardware) msi_first=0x20000 and msi_last=0x2ffff

		// the first and last values obtained from from enable_msi(&first, &last) refers to the address range 
		// available to an etherbone master on a host connected to the pcie-wishbone bridge driver
		// for example, if two processes are connected to dev/wbm0, 
		// the first gets:  first = 0x00000 and last = 0x00fff
		// the second gets: first = 0x01000 and last = 0x01fff
		//
		// if a hardware MSI master writes to 0x10400, the first  etherbone master (connected to dev/wbm0) will get a callback on address 0x400
		// if a hardware MSI master writes to 0x11400, the second etherbone master (connected to dev/wbm0) will get a callback on address 0x400
		// if a hardware MSI master writes to 0x20400, another etherbone master connected via usb will get the MSI on address 0x400
		if ((sdb.msi_first & size_mask) != 0) {
			throw etherbone::exception_t("request_irq/misaligned", EB_FAIL);
		}

		std::cerr << "first = 0x" << std::hex << std::setw(8) << std::setfill('0') << first << std::endl;
		std::cerr << "last  = 0x" << std::hex << std::setw(8) << std::setfill('0') << last  << std::endl;
		std::cerr << "eca msi_first = 0x" << std::hex << std::setw(8) << std::setfill('0') << sdb.msi_first << std::endl;
		std::cerr << "eca msi_last  = 0x" << std::hex << std::setw(8) << std::setfill('0') << sdb.msi_last  << std::endl;

		// values from enable_msi(&first, &last);
		eb_address_t base = first;
		eb_address_t mask = last-first;

		// Confirm that the MSI range could contain our master (not mismapped)
		if (size_mask < mask) {
			throw etherbone::exception_t("request_irq/badly_mapped", EB_FAIL);
		}

		// make up and irq address and connect a callback
		while (true) {
			// Select an IRQ
			eb_address_t irq = ((rand() & mask) + base) & (~0x3);
			// try to attach
			if ( saftd->request_irq(irq, std::bind(&EcaDriver::msiHandler, this, std::placeholders::_1, channel_idx)) ) {
				std::cerr << "registered irq under address " << std::hex << std::setw(8) << std::setfill('0') << irq 
				          << std::dec
				          << std::endl;
				channel_msis.push_back(irq);
				// configure the output channel with the chosen irq address
				setHandler(channel_idx, true, channel_msis.back() + sdb.msi_first);
				break;
			}
		}
	}

}

EcaDriver::~EcaDriver() 
{
	if (container) {
		for (auto &channel: ECAchannels) {
			for (auto &actionSink: channel) {
				if (actionSink) {
					std::cerr << "   remove " << actionSink->getObjectPath() << std::endl;
					container->remove_object(actionSink->getObjectPath());
				}
			}
		}
	}
}


void EcaDriver::setHandler(unsigned channel, bool enable, eb_address_t address)
{
  etherbone::Cycle cycle;
  cycle.open(device);
  cycle.write(base + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
  cycle.write(base + ECA_CHANNEL_MSI_SET_ENABLE_OWR, EB_DATA32, 0);
  cycle.write(base + ECA_CHANNEL_MSI_SET_TARGET_OWR, EB_DATA32, address);
  cycle.write(base + ECA_CHANNEL_MSI_SET_ENABLE_OWR, EB_DATA32, enable?1:0);
  cycle.close();
  // clog << kLogDebug << "TimingReceiver: registered irq 0x" << std::hex << address << std::endl;
}

void EcaDriver::msiHandler(eb_data_t msi, unsigned channel)
{
	std::cerr << "TimingReceiver::msiHandler " << msi << " " << channel << std::endl;
	unsigned code = msi >> 16;
	unsigned num  = msi & 0xFFFF;

	std::cerr << "MSI: " << channel << " " << num << " " << code << std::endl;

	// MAX_FULL is tracked by this object, not the subchannel
	if (code == ECA_MAX_FULL) {
		updateMostFull(channel);
	} else {
		auto &chan = ECAchannels[channel];
		if (num >= chan.size()) {
			std::cerr << "MSI for non-existant channel " << channel << " num " << num << std::endl;
		} else {
			auto &actionSink = chan[num];
			if (actionSink) {
				actionSink->receiveMSI(code);
			} else {
				// It could be that the user deleted the SoftwareActionSink
				// while it still had pending actions. Pop any stale records.
				if (code == ECA_VALID) popMissingQueue(channel, num);
			}
		}
	}
}

void EcaDriver::InjectEvent(uint64_t event, uint64_t param, eb_plugin::Time time)
{
	etherbone::Cycle cycle;

	cycle.open(device);
	cycle.write(stream, EB_DATA32, event >> 32);
	cycle.write(stream, EB_DATA32, event & 0xFFFFFFFFUL);
	cycle.write(stream, EB_DATA32, param >> 32);
	cycle.write(stream, EB_DATA32, param & 0xFFFFFFFFUL);
	cycle.write(stream, EB_DATA32, 0); // reserved
	cycle.write(stream, EB_DATA32, 0); // TEF
	cycle.write(stream, EB_DATA32, time.getTAI() >> 32);
	cycle.write(stream, EB_DATA32, time.getTAI() & 0xFFFFFFFFUL);
	cycle.close();
}


uint16_t EcaDriver::updateMostFull(unsigned channel)
{
	if (channel >= most_full.size()) return 0;

	etherbone::Cycle cycle;
	eb_data_t raw;

	cycle.open(device);
	cycle.write(base + ECA_CHANNEL_SELECT_RW,        EB_DATA32, channel);
	cycle.read (base + ECA_CHANNEL_MOSTFULL_ACK_GET, EB_DATA32, &raw);
	cycle.close();

	uint16_t mostFull = raw & 0xFFFF;
	uint16_t used     = raw >> 16;

	if (most_full[channel] != mostFull) {
		most_full[channel] = mostFull;

		// // Broadcast new value to all subchannels
		// maybe introduce a signal for this (is anyone using this?)
		// SinkKey low(channel, 0);
		// SinkKey high(channel+1, 0);
		// ActionSinks::iterator first = actionSinks.lower_bound(low);
		// ActionSinks::iterator last  = actionSinks.lower_bound(high);
	}

	return used;
}

void EcaDriver::resetMostFull(unsigned channel)
{
	if (channel >= most_full.size()) return;

	etherbone::Cycle cycle;
	eb_data_t null;

	cycle.open(device);
	cycle.write(base + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
	cycle.read (base + ECA_CHANNEL_MOSTFULL_CLEAR_GET, EB_DATA32, &null);
	cycle.close();
}

void EcaDriver::popMissingQueue(unsigned channel, unsigned num)
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


void EcaDriver::compile()
{
	std::cerr << "TimingReceiver::compile" << std::endl;
  // Store all active conditions into a vector for processing
  typedef std::vector<ECA_OpenClose> ID_Space;
  ID_Space id_space;

	// Step one is to find all active conditions on all action sinks
	for (auto &channel: ECAchannels) {
		for (auto &actionSink: channel) {
			for (auto &number_condition: actionSink->getConditions()) {
				auto &condition = number_condition.second;
				if (!condition->getActive()) continue;

				// Memorize the condition
				ECA_OpenClose oc;
				oc.key     = condition->getID() &  condition->getMask();
				oc.open    = true;
				oc.subkey  = condition->getID() | ~condition->getMask();
				oc.offset  = condition->getOffset();
				oc.tag     = condition->getRawTag();
				oc.flags   =(condition->getAcceptLate()    ?(1<<ECA_LATE)    :0) |
				            (condition->getAcceptEarly()   ?(1<<ECA_EARLY)   :0) |
				            (condition->getAcceptConflict()?(1<<ECA_CONFLICT):0) |
				            (condition->getAcceptDelayed() ?(1<<ECA_DELAYED) :0);
				oc.channel = actionSink->getChannel();
				oc.num     = actionSink->getNum();

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


SoftwareActionSink *EcaDriver::getSoftwareActionSink(const std::string & sas_obj_path)
{
	for (auto &softwareActionSink: *ECA_LINUX_channel) {
		if (softwareActionSink->getObjectPath() == sas_obj_path) {
			return dynamic_cast<SoftwareActionSink*>(softwareActionSink.get());
		}
	}
	throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such device");
}


etherbone::Device &EcaDriver::get_device() {
	return device;
}

static inline bool not_isalnum_(char c)
{
	return !(isalnum(c) || c == '_');
}

std::string EcaDriver::NewSoftwareActionSink(const std::string& name_)
{
	if (container) {
		std::cerr << "TimingReceiver::NewSoftwareActionSink client_id = " << container->get_calling_client_id() << std::endl;
	}
	std::cerr << "A" << std::endl;

	if (ECA_LINUX_channel == nullptr) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "ECA has no available linux-facing queues");
	}
	std::cerr << "A" << std::endl;
	if (ECA_LINUX_channel->size() >= ECA_LINUX_channel_subchannels) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "All available Linux facing ECA subchannels are already in use");
	}
	std::cerr << "A" << std::endl;

	// build a name. For SoftwareActionSinks it always starts with "software/<name>"
	std::string name("software/");
	std::cerr << "A" << std::endl;
	if (name_ == "") {
	std::cerr << "A" << std::endl;
		// if no name is provided, we generate one 
		std::string seq;
		std::ostringstream str;
		str.imbue(std::locale("C"));
		str << "_" << ++sas_count;
		seq = str.str();
		name.append(seq);
	} else {
	std::cerr << "A" << std::endl;
		if (name_[0] == '_') {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; leading _ is reserved");
		}
		if (find_if(name_.begin(), name_.end(), not_isalnum_) != name_.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
		}

		std::map< std::string, std::string > sinks = getSoftwareActionSinks();
		if (sinks.find(name_) != sinks.end()) {
			throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Name already in use");
		}
		name.append(name_);
	}
	std::cerr << "A" << std::endl;
  
	unsigned channel = ECA_LINUX_channel_index;
	std::cerr << "A" << std::endl;
	unsigned num     = ECA_LINUX_channel->size(); 
  	std::cerr << "channel = " << channel << " num = " << num << " queue_addresses.size() = " << queue_addresses.size() << std::endl;
	eb_address_t address = queue_addresses[channel];
	std::cerr << "A" << std::endl;

std::cerr << object_path  << "  /  " << name << std::endl;

	std::unique_ptr<SoftwareActionSink> software_action_sink(new SoftwareActionSink(this, name, channel, num, address, container));
	std::cerr << "A" << std::endl;
	std::string sink_object_path = software_action_sink->getObjectPath();
	std::cerr << "A" << std::endl;
	if (container) {
	std::cerr << "A" << std::endl;
		std::unique_ptr<SoftwareActionSink_Service> service(new SoftwareActionSink_Service(software_action_sink.get(), std::bind(&EcaDriver::removeSowftwareActionSink,this, software_action_sink.get())));
	std::cerr << "A" << std::endl;
		container->set_owner(service.get());
	std::cerr << "A" << std::endl;
		container->create_object(sink_object_path, std::move(service));
	}
	std::cerr << "A" << std::endl;
	ECA_LINUX_channel->push_back(std::move(software_action_sink));
	std::cerr << "A" << std::endl;

	return sink_object_path;
}

bool operator==(const std::unique_ptr<ActionSink> &up, const ActionSink * p) {
	return up.get() == p;
}
void EcaDriver::removeSowftwareActionSink(SoftwareActionSink *sas) {
	ActionSink *as = sas;
	ECA_LINUX_channel->erase(std::remove(ECA_LINUX_channel->begin(), ECA_LINUX_channel->end(), as),
		                     ECA_LINUX_channel->end());
}

std::map< std::string, std::string > EcaDriver::getSoftwareActionSinks() const
{
	std::map< std::string, std::string > out;
	if (ECA_LINUX_channel != nullptr) {
		for (auto &softwareActionSink: *ECA_LINUX_channel) {
			out[softwareActionSink->getObjectName()] = softwareActionSink->getObjectPath();
		}
	}
	return out;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


// bool TimingReceiver::aquire_watchdog() {
// 	// try to acquire watchdog
// 	eb_data_t retry;
// 	device.read(watchdog, EB_DATA32, &watchdog_value);
// 	if ((watchdog_value & 0xFFFF) != 0) {
// 		return false;
// 	}
// 	device.write(watchdog, EB_DATA32, watchdog_value);
// 	device.read(watchdog, EB_DATA32, &retry);
// 	if (((retry ^ watchdog_value) >> 16) != 0) {
// 		return false;
// 	}
// 	return true;
// }

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
	pps->getLocked();
	watchdog->update();
	// device.write(watchdog, EB_DATA32, watchdog_value);
	return true;
}


TimingReceiver::TimingReceiver(SAFTd *sd, const std::string &n, const std::string eb_path, saftbus::Container *cont)
	: saftd(sd)
	, object_path(sd->get_object_path() + "/" + n)
	, name(n)
	, etherbone_path(eb_path)
	// , sas_count(0)
	, container(cont)
	// , locked(false)
{
	std::cerr << "TimingReceiver::TimingReceiver" << std::endl;

	if (find_if(name.begin(), name.end(), [](char c){ return !(isalnum(c) || c == '_');} ) != name.end()) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Invalid name; [a-zA-Z0-9_] only");
	}

	stat(etherbone_path.c_str(), &dev_stat);
	device.open(sd->get_etherbone_socket(), etherbone_path.c_str());

	try {
		watchdog = std::move(std::unique_ptr<WatchdogDriver>(new WatchdogDriver(device)));
		pps      = std::move(std::unique_ptr<PpsDriver>     (new PpsDriver     (device, SigLocked)));
		eca      = std::move(std::unique_ptr<EcaDriver>     (new EcaDriver     (saftd, device, object_path, container)));
	} catch (etherbone::exception_t &e) {
		device.close();
		chmod(etherbone_path.c_str(), dev_stat.st_mode);
		throw;		
	}

	std::vector<etherbone::sdb_msi_device> /*ecas_dev, */ mbx_msi_dev;
	std::vector<sdb_device> /*streams_dev, */infos_dev, /*watchdogs_dev,*/ scubus_dev, /*pps_dev,*/ /*mbx_dev,*/ ats_dev;
	eb_address_t ats_addr = 0; // not every Altera FPGA model has a temperature sensor, i.e, Altera II

	// device.sdb_find_by_identity_msi(ECA_SDB_VENDOR_ID, ECA_SDB_DEVICE_ID, ecas_dev);
	device.sdb_find_by_identity_msi(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx_msi_dev);

	// device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x8752bf45, streams_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x2d39fa8b, infos_dev);
	// device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0xb6232cd3, watchdogs_dev);
	device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x9602eb6f, scubus_dev);
	// device.sdb_find_by_identity(0xce42, 0xde0d8ced, pps_dev);
	device.sdb_find_by_identity(ATS_SDB_VENDOR_ID,  ATS_SDB_DEVICE_ID, ats_dev);
	// device.sdb_find_by_identity(MSI_MAILBOX_VENDOR, MSI_MAILBOX_PRODUCT, mbx_dev);

	// only support super basic hardware for now
	if (/*ecas_dev.size() != 1 ||*/ /*streams_dev.size() != 1 ||*/ infos_dev.size() != 1 /*|| watchdogs_dev.size() != 1 */
		|| /*pps_dev.size() != 1 ||*/ /*mbx_dev.size() != 1 || */mbx_msi_dev.size() != 1) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has insuficient hardware resources");
	}

	// std::cerr << "ecas.msi_first=" << std::hex << std::setw(8) << std::setfill('0') << ecas_dev[0].msi_first 
	//           << "     msi_last="  << std::hex << std::setw(8) << std::setfill('0') << ecas_dev[0].msi_last
	//           << std::dec
	//           << std::endl;

	std::cerr << "mbox.msi_first=" << std::hex << std::setw(8) << std::setfill('0') << mbx_msi_dev[0].msi_first 
	          << "     msi_last="  << std::hex << std::setw(8) << std::setfill('0') << mbx_msi_dev[0].msi_last
	          << std::dec
	          << std::endl;

	// base      = ecas_dev[0].sdb_component.addr_first;
 //    stream    = (eb_address_t)streams_dev[0].sdb_component.addr_first;
    info      = (eb_address_t)infos_dev[0].sdb_component.addr_first;
    // watchdog  = (eb_address_t)watchdogs_dev[0].sdb_component.addr_first;
    // pps       = (eb_address_t)pps_dev[0].sdb_component.addr_first;

// mbox_for_testing_only = (eb_address_t)mbx_dev[0].sdb_component.addr_first;

  //   if (!aquire_watchdog()) {
		// throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Timing Receiver already locked");
  //   }
	setupGatewareInfo(info);

	// update locked status ...
	poll();
	//    ... and repeat every 1s 
	poll_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
			std::bind(&TimingReceiver::poll, this), std::chrono::milliseconds(1000), std::chrono::milliseconds(1000)
		);
}



TimingReceiver::~TimingReceiver() 
{
	std::cerr << "TimingReceiver::~TimingReceiver" << std::endl;



	std::cerr << "saftbus::Loop::get_default().remove(poll_timeout_source)" << std::endl;
	saftbus::Loop::get_default().remove(poll_timeout_source);
	std::cerr << "device.close()" << std::endl;
	device.close();
	std::cerr << "fix device file" << std::endl;
	chmod(etherbone_path.c_str(), dev_stat.st_mode);

}


// void TimingReceiver::setHandler(unsigned channel, bool enable, eb_address_t address)
// {
//   etherbone::Cycle cycle;
//   cycle.open(device);
//   cycle.write(base + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
//   cycle.write(base + ECA_CHANNEL_MSI_SET_ENABLE_OWR, EB_DATA32, 0);
//   cycle.write(base + ECA_CHANNEL_MSI_SET_TARGET_OWR, EB_DATA32, address);
//   cycle.write(base + ECA_CHANNEL_MSI_SET_ENABLE_OWR, EB_DATA32, enable?1:0);
//   cycle.close();
//   // clog << kLogDebug << "TimingReceiver: registered irq 0x" << std::hex << address << std::endl;
// }

// void TimingReceiver::msiHandler(eb_data_t msi, unsigned channel)
// {
// 	std::cerr << "TimingReceiver::msiHandler " << msi << " " << channel << std::endl;
// 	unsigned code = msi >> 16;
// 	unsigned num  = msi & 0xFFFF;

// 	std::cerr << "MSI: " << channel << " " << num << " " << code << std::endl;

// 	// MAX_FULL is tracked by this object, not the subchannel
// 	if (code == ECA_MAX_FULL) {
// 		updateMostFull(channel);
// 	} else {
// 		auto &chan = ECAchannels[channel];
// 		if (num >= chan.size()) {
// 			std::cerr << "MSI for non-existant channel " << channel << " num " << num << std::endl;
// 		} else {
// 			auto &actionSink = chan[num];
// 			if (actionSink) {
// 				actionSink->receiveMSI(code);
// 			} else {
// 				// It could be that the user deleted the SoftwareActionSink
// 				// while it still had pending actions. Pop any stale records.
// 				if (code == ECA_VALID) popMissingQueue(channel, num);
// 			}
// 		}
// 	}
// }








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

// #define WR_API_VERSION          4

// #if WR_API_VERSION <= 3
// #define WR_PPS_GEN_ESCR_MASK    0x6       //bit 1: PPS valid, bit 2: TS valid
// #else
// #define WR_PPS_GEN_ESCR_MASK    0xc       //bit 2: PPS valid, bit 3: TS valid
// #endif
// #define WR_PPS_GEN_ESCR         0x1c      //External Sync Control Register


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
	return pps->getLocked();
	// eb_data_t data;
	// device.read(pps + WR_PPS_GEN_ESCR, EB_DATA32, &data);
	// bool newLocked = (data & WR_PPS_GEN_ESCR_MASK) == WR_PPS_GEN_ESCR_MASK;

	// /* Update signal */
	// if (newLocked != locked) {
	// 	locked = newLocked;
	// 	if (SigLocked) {
	// 		SigLocked(locked);
	// 	}
	// }

	// // just to see if msis work: use MBOX to send us a telegram
	// device.write(mbox_for_testing_only + 4, EB_DATA32, 0x10f00); // configure MSI
	// device.write(mbox_for_testing_only + 0, EB_DATA32, 0xaffe);  // send MSI

	// return newLocked;
}



void TimingReceiver::InjectEvent(uint64_t event, uint64_t param, uint64_t time)
{
	// InjectEvent(event,param,eb_plugin::makeTimeTAI(time));
	eca->InjectEvent(event,param,eb_plugin::makeTimeTAI(time));
}

void TimingReceiver::InjectEvent(uint64_t event, uint64_t param, eb_plugin::Time time)
{
// 	etherbone::Cycle cycle;

// 	cycle.open(device);
// 	cycle.write(stream, EB_DATA32, event >> 32);
// 	cycle.write(stream, EB_DATA32, event & 0xFFFFFFFFUL);
// 	cycle.write(stream, EB_DATA32, param >> 32);
// 	cycle.write(stream, EB_DATA32, param & 0xFFFFFFFFUL);
// 	cycle.write(stream, EB_DATA32, 0); // reserved
// 	cycle.write(stream, EB_DATA32, 0); // TEF
// 	cycle.write(stream, EB_DATA32, time.getTAI() >> 32);
// 	cycle.write(stream, EB_DATA32, time.getTAI() & 0xFFFFFFFFUL);
// 	cycle.close();

	eca->InjectEvent(event, param, time);
}


std::string TimingReceiver::NewSoftwareActionSink(const std::string& name) {
	return eca->NewSoftwareActionSink(name);
}

std::map< std::string, std::string > TimingReceiver::getSoftwareActionSinks() const {
	return eca->getSoftwareActionSinks();
}

SoftwareActionSink *TimingReceiver::getSoftwareActionSink(const std::string & object_path) {
	return eca->getSoftwareActionSink(object_path);
}


uint64_t TimingReceiver::ReadRawCurrentTime()
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

	return uint64_t(time1) << 32 | time0;
}
eb_plugin::Time TimingReceiver::CurrentTime()
{
	if (!pps->locked) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "TimingReceiver is not Locked");
	}

	return eb_plugin::makeTimeTAI(ReadRawCurrentTime());
}



} // namespace saftlib


