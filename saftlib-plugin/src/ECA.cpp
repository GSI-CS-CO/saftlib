#include "ECA.hpp"
#include "SAFTd.hpp"

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <cassert>

#include <saftbus/error.hpp>
#include <saftbus/service.hpp>

#include "SoftwareActionSink.hpp"
#include "SoftwareActionSink_Service.hpp"
#include "SCUbusActionSink.hpp"
#include "SCUbusActionSink_Service.hpp"
#include "EmbeddedCPUActionSink.hpp"
#include "EmbeddedCPUActionSink_Service.hpp"
#include "WbmActionSink.hpp"
#include "WbmActionSink_Service.hpp"
#include "Output.hpp"

#include "eca_regs.h"
#include "eca_flags.h"
#include "eca_queue_regs.h"
#include "fg_regs.h"
#include "ats_regs.h"


namespace saftlib {


#define EVENT_SDB_DEVICE_ID             0x8752bf45





uint16_t ECA::updateMostFull(unsigned channel)
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
	}

	return used;
}

void ECA::resetMostFull(unsigned channel)
{
	if (channel >= most_full.size()) return;

	etherbone::Cycle cycle;
	eb_data_t null;

	cycle.open(device);
	cycle.write(base + ECA_CHANNEL_SELECT_RW,          EB_DATA32, channel);
	cycle.read (base + ECA_CHANNEL_MOSTFULL_CLEAR_GET, EB_DATA32, &null);
	cycle.close();
}

void ECA::popMissingQueue(unsigned channel, unsigned num)
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




void ECA::probeConfiguration() 
{
	std::vector<etherbone::sdb_msi_device> ecas_dev;
	std::vector<sdb_device> streams_dev;

	std::cerr << "A" << std::endl;
	device.sdb_find_by_identity_msi(ECA_SDB_VENDOR_ID, ECA_SDB_DEVICE_ID, ecas_dev);
	std::cerr << "B" << std::endl;
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

	// This just reads the MSI address range out of the ehterbone config space registers
	// It does not actually enable anything ... MSIs also work without this
	device.enable_msi(&first, &last);
	std::cerr << "TimingReceiver enable_msi first=0x" << std::hex << std::setw(8) << std::setfill('0') << first 
	          <<                           " last=0x" << std::hex << std::setw(8) << std::setfill('0') << last << std::endl;

	// Confirm the device is an aligned power of 2
	eb_address_t size = last - first;
	if (((size + 1) & size) != 0) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has strange sized MSI range");
	}
	if ((first & size) != 0) {
		throw saftbus::Error(saftbus::Error::IO_ERROR, "Device has unaligned MSI first address");
	}

	etherbone::sdb_msi_device& sdb = ecas_dev[0];

	msi_first = sdb.msi_first;
	msi_last = sdb.msi_last;

	// Confirm we had SDB records for MSI all the way down
	if (msi_last < msi_first) {
		throw etherbone::exception_t("request_irq/non_msi_crossbar_inbetween", EB_FAIL);
	}

	// Confirm that first is aligned to size
	// e.g. first = 0x10000  last = 0x1ffff 
	// => size_mask = 0x0ffff
	// => 0x10000 & 0x0ffff = 0
	eb_address_t size_mask = msi_last - msi_first;
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
	if ((msi_first & size_mask) != 0) {
		throw etherbone::exception_t("request_irq/misaligned", EB_FAIL);
	}

	std::cerr << "first = 0x" << std::hex << std::setw(8) << std::setfill('0') << first << std::endl;
	std::cerr << "last  = 0x" << std::hex << std::setw(8) << std::setfill('0') << last  << std::endl;
	std::cerr << "eca msi_first = 0x" << std::hex << std::setw(8) << std::setfill('0') << msi_first << std::endl;
	std::cerr << "eca msi_last  = 0x" << std::hex << std::setw(8) << std::setfill('0') << sdb.msi_last  << std::endl;


	// Probe the configuration of the ECA
	eb_data_t raw_channels, raw_search, raw_walker;
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
}

void ECA::prepareChannels()
{
	eb_data_t raw_type, raw_max_num, raw_capacity;
	etherbone::Cycle cycle;

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
					ECA_LINUX_channel->resize(raw_max_num); // create enough slots for as much SoftwareActionSinks as the hardware supports
				} else {
					std::cerr << "more than one Linux facing ECA channel. We will use the first of them and ignore the rest" << std::endl;
				}
			break;
			case ECA_WBM: {
				std::cerr << "============== FOUND WBM ACTION SINK object_path = " << object_path << std::endl;
				std::vector<sdb_device> acwbm;
				device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x18415778, acwbm);
				if (acwbm.size() == 1) {
					std::string path = object_path + "/acwbm";

					std::unique_ptr<WbmActionSink> wbm_sink(new WbmActionSink( device, *this, path, "acwbm", channel_idx, (eb_address_t)acwbm[0].sdb_component.addr_first, container  ));
					if (container) {
						std::unique_ptr<WbmActionSink_Service> service(new WbmActionSink_Service(wbm_sink.get()));
						container->create_object(path, std::move(service));
					}
					ECAchannels[channel_idx].push_back(std::move(wbm_sink));
					wbm_action_sinks["acwbm"] = path;
				}
			} break;
			case ECA_SCUBUS: {
				std::cerr << "============== FOUND SCU_BUS ACTION SINK object_path = " << object_path << std::endl;
				std::vector<sdb_device> scubus;
				device.sdb_find_by_identity(ECA_SDB_VENDOR_ID, 0x9602eb6f, scubus);
				if (scubus.size() == 1) {
					std::string path = object_path + "/scubus";

					std::unique_ptr<SCUbusActionSink> scubus_sink(new SCUbusActionSink( device, *this, path, "scubus", channel_idx, (eb_address_t)scubus[0].sdb_component.addr_first, container  ));
					if (container) {
						std::unique_ptr<SCUbusActionSink_Service> service(new SCUbusActionSink_Service(scubus_sink.get()));
						container->create_object(path, std::move(service));
					}
					ECAchannels[channel_idx].push_back(std::move(scubus_sink));
					scubus_action_sinks["scubus"] = path;
				}
			} break;
			case ECA_EMBEDDED_CPU: {
				std::cerr << "ECA: Found queue..." << std::endl;
				for (unsigned queue_id = 1; queue_id < channels; ++queue_id) {
					eb_data_t get_id;
					cycle.open(device);
					cycle.read(queue_addresses[queue_id]+ECA_QUEUE_QUEUE_ID_GET, EB_DATA32, &get_id);
					cycle.close();
					std::cerr << "ECA: Found queue @ 0x" << std::hex << queue_addresses[queue_id] << std::dec << std::endl;
					std::cerr << "ECA: Found queue with ID: " << get_id << std::endl;
					if (get_id == ECA_EMBEDDED_CPU) {
						std::cerr << "ECA: Found embedded CPU channel!" << std::endl;
						std::string path = object_path + "/embedded_cpu";

						std::unique_ptr<EmbeddedCPUActionSink> ecpu_sink(new EmbeddedCPUActionSink(*this, path, "embedded_cpu", channel_idx, container));
						if (container) {
							std::unique_ptr<EmbeddedCPUActionSink_Service> service(new EmbeddedCPUActionSink_Service(ecpu_sink.get()));
							container->create_object(path, std::move(service));
						}
						ECAchannels[channel_idx].push_back(std::move(ecpu_sink));
						ecpu_action_sinks["embedded_cpu"] = path;
					}
				}
			}			
			break;
			// case ECA_EMBEDDED_CPU:
			// break;
		}
	}
}

void ECA::setMsiHandlers(SAFTd &saftd) 
{
	// set MSI handlers

	for (unsigned channel_idx = 0; channel_idx < channels; ++channel_idx) {


		// values from enable_msi(&first, &last);
		eb_address_t base = first;
		eb_address_t mask = last-first;

		eb_address_t msi_mask = msi_last-msi_first;

		// Confirm that the MSI range could contain our master (not mismapped)
		if (msi_mask < mask) {
			throw etherbone::exception_t("request_irq/badly_mapped", EB_FAIL);
		}

		// make up and irq address and connect a callback
		while (true) {
			// Select an IRQ
			eb_address_t irq = ((rand() & mask) + base) & (~0x3);
			// try to attach
			if ( saftd.request_irq(irq, std::bind(&ECA::msiHandler, this, std::placeholders::_1, channel_idx)) ) {
				std::cerr << "registered irq under address " << std::hex << std::setw(8) << std::setfill('0') << irq 
									<< std::dec
									<< std::endl;
				channel_msis.push_back(irq);
				// configure the output channel with the chosen irq address
				setHandler(channel_idx, true, channel_msis.back() + msi_first);
				break;
			}
		}
	}
}


void ECA::msiHandler(eb_data_t msi, unsigned channel)
{
	// std::cerr << "TimingReceiver::msiHandler " << msi << " " << channel << std::endl;
	unsigned code = msi >> 16;
	unsigned num  = msi & 0xFFFF;

	// std::cerr << "MSI: " << channel << " " << num << " " << code << std::endl;

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

void ECA::setHandler(unsigned channel, bool enable, eb_address_t address)
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



void ECA::compile()
{
	// std::cerr << "ECA::compile" << std::endl;
	// Store all active conditions into a vector for processing
	typedef std::vector<ECA_OpenClose> ID_Space;
	ID_Space id_space;

	// Step one is to find all active conditions on all action sinks
	for (auto &channel: ECAchannels) {
		for (auto &actionSink: channel) {
			if (!actionSink) {
				continue;
			}
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

				// std::cerr << "compile condition on channel " << oc.channel << " num " << oc.num << std::endl;

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
	
// #if DEBUG_COMPILE
// 	clog << kLogDebug << "Table compilation complete!" << std::endl;
// 	for (i = 0; i < search.size(); ++i)
// 		clog << kLogDebug << "S: " << search[i].event << " " << search[i].index << std::endl;
// 	for (i = 0; i < walk.size(); ++i)
// 		clog << kLogDebug << "W: " << walk[i].next << " " << walk[i].offset << " " << walk[i].tag << " " << walk[i].flags << " " << (int)walk[i].channel << " " << (int)walk[i].num << std::endl;
// #endif

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



////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


void attach_io_control(const IoControl &io_control) 
{

}



uint16_t ECA::getMostFull(int channel)
{
	return most_full[channel];
}

const std::string &ECA::get_object_path()
{
	return object_path;
}


// void ECA::compile() 
// {
// 	compile();
// }


ECA::ECA(SAFTd &saftd, etherbone::Device &dev, const std::string &obj_path, saftbus::Container *cont)
	: device(dev)
	, object_path(obj_path)
	, container(cont)
	, sas_count(0)
{
	// std::cerr << "ECA::ECA() object_path " << object_path << std::endl;
	probeConfiguration();
	compile(); // remove old rules
	prepareChannels();
	setMsiHandlers(saftd); // hook
}

ECA::~ECA() 
{
	// std::cerr << "ECA::~ECA()" << std::endl;
	if (container) {
		for (auto &channel: ECAchannels) {
			for (auto &actionSink: channel) {
				if (actionSink) {
					// std::cerr << "   remove " << actionSink->getObjectPath() << std::endl;
					try {
						container->remove_object(actionSink->getObjectPath());
					} catch (saftbus::Error &e) {
						// std::cerr << "removal attempt failed: " << e.what() << std::endl;
					}
				}
			}
		}
	}
}

bool ECA::addActionSink(int channel, std::unique_ptr<ActionSink> sink) 
{
	assert(channel >= 0 && channel < static_cast<int>(ECAchannels.size()));
	ECAchannels[channel].push_back(std::move(sink));
	return true;
}

eb_address_t ECA::get_base_address() 
{
	return base;
}

uint64_t ECA::ReadRawCurrentTime()
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



void ECA::InjectEventRaw(uint64_t event, uint64_t param, uint64_t time)
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







SoftwareActionSink *ECA::getSoftwareActionSink(const std::string & sas_obj_path)
{
	std::cerr << "getSoftwareActionSink " << ECA_LINUX_channel->size() << std::endl;
	for (auto &softwareActionSink: *ECA_LINUX_channel) {
		if (!softwareActionSink) {
			std::cerr << "." << std::endl;
			continue;
		}
		std::cerr << "+" << std::endl;
		if (softwareActionSink->getObjectPath() == sas_obj_path) {
			std::cerr << "return " << softwareActionSink->getObjectPath() << std::endl;
			return dynamic_cast<SoftwareActionSink*>(softwareActionSink.get());
		}
	}
	throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such SoftwareActionSink");
}


Output *ECA::getOutput(const std::string &output_obj_path)
{
	for (auto &output: ECAchannels[0]) { // outputs are always on channel 0
		std::cerr << "getOutput " << output->getObjectPath() << " " << output_obj_path << std::endl;
		if (output->getObjectPath() == output_obj_path) {
			return dynamic_cast<Output*>(output.get());
		}
	}
	throw saftbus::Error(saftbus::Error::INVALID_ARGS, "no such Output");
}



etherbone::Device &ECA::get_device() {
	return device;
}

static inline bool not_isalnum_(char c)
{
	return !(isalnum(c) || c == '_');
}

std::string ECA::NewSoftwareActionSink(const std::string& name_)
{
	if (container) {
		std::cerr << "TimingReceiver::NewSoftwareActionSink client_id = " << container->get_calling_client_id() << std::endl;
	}

	if (ECA_LINUX_channel == nullptr) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "ECA has no available linux-facing queues");
	}

	// find the first free slot in ECA_LINUX_channel
	std::cerr << "ECA::NewSoftwareActionSink:   find 1 in " << std::dec << ECA_LINUX_channel->size() << "slots" << std::endl;
	unsigned num = 0;
	for (auto &softwareActionSink: *ECA_LINUX_channel) {
		if (!softwareActionSink) break;
		++num;
	}
	if (num >= ECA_LINUX_channel_subchannels) {
		throw saftbus::Error(saftbus::Error::INVALID_ARGS, "All available Linux facing ECA subchannels are already in use");
	}

	// build a name. For SoftwareActionSinks it always starts with "software/<name>"
	std::string name;
	if (name_ == "") {
		// if no name is provided, we generate one 
		std::string seq;
		std::ostringstream str;
		str.imbue(std::locale("C"));
		str << "_" << ++sas_count;
		seq = str.str();
		name.append(seq);
	} else {
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
	
	unsigned channel = ECA_LINUX_channel_index;
	std::cerr << "NewSoftwareActionSink: channel = " << channel << " num = " << num << " queue_addresses.size() = " << queue_addresses.size() << std::endl;
	eb_address_t address = queue_addresses[channel];

	std::string path = object_path;
	path.append("/software/");
	path.append(name);
	std::unique_ptr<SoftwareActionSink> software_action_sink(
		new SoftwareActionSink(*this, 
			path, 
			name, 
			channel, 
			num, 
			address, 
			container));
	std::string sink_object_path = software_action_sink->getObjectPath();
	if (container) {
		std::unique_ptr<SoftwareActionSink_Service> service(new SoftwareActionSink_Service(software_action_sink.get(), std::bind(&ECA::removeSowftwareActionSink,this, software_action_sink.get())));
		container->set_owner(service.get());
		container->create_object(sink_object_path, std::move(service));
	}
	(*ECA_LINUX_channel)[num] = std::move(software_action_sink);

	return sink_object_path;
}

bool operator==(const std::unique_ptr<ActionSink> &up, const ActionSink * p) {
	return up.get() == p;
}
void ECA::removeSowftwareActionSink(SoftwareActionSink *sas) {
	ActionSink *as = sas;
	std::cout << "========= removeSowftwareActionSink ======== " << as->getNum() << std::endl;
	(*ECA_LINUX_channel)[as->getNum()].reset();
	compile();
}

std::map< std::string, std::string > ECA::getSoftwareActionSinks() const
{
	std::map< std::string, std::string > out;
	if (ECA_LINUX_channel != nullptr) {
		for (auto &softwareActionSink: *ECA_LINUX_channel) {
			if (softwareActionSink) {
				out[softwareActionSink->getObjectName()] = softwareActionSink->getObjectPath();
			}
		}
	}
	return out;
}

std::map< std::string, std::string > ECA::getSCUbusActionSinks() const 
{
	return scubus_action_sinks;
}

std::map< std::string, std::string > ECA::getEmbeddedCPUActionSinks() const 
{
	return ecpu_action_sinks;
}

std::map< std::string, std::string > ECA::getWbmActionSinks() const 
{
	return wbm_action_sinks;
}


std::map< std::string, std::string > ECA::getOutputs() const
{
	std::map< std::string, std::string > out;
	assert(ECAchannels.size() > 0);
	for (auto &action_sink: ECAchannels[0]) {
		Output* output = dynamic_cast<Output*>(action_sink.get());
		if (output) {
		    out[output->getObjectName()] = output->getObjectPath();
		}
	}
	return out;
}

uint32_t ECA::getFree() const 
{
	return max_conditions - used_conditions;
}


} // namespace

