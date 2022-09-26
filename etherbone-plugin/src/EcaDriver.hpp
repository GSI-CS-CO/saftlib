#ifndef EB_PLUGIN_ECA_DRIVER_HPP_
#define EB_PLUGIN_ECA_DRIVER_HPP_


#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>

#include <saftbus/error.hpp>

#include "Time.hpp"
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

class EcaDriver {
	friend class ActionSink;

	SAFTd *saftd;
	etherbone::Device  &device;
	const std::string  &object_path;
	saftbus::Container *container;

	uint64_t sas_count; // number of SoftwareActionSinks

	eb_address_t base;
	eb_address_t stream;

	// Msi address range
	eb_address_t first;
	eb_address_t last; 



	unsigned channels;
	unsigned search_size;
	unsigned walker_size;
	unsigned max_conditions;
	unsigned used_conditions;
	std::vector<eb_address_t> channel_msis;
	std::vector<eb_address_t> queue_addresses;
	std::vector<uint16_t> most_full;

	// public type, even though the member is private
	typedef std::pair<unsigned, unsigned> SinkKey; // (channel, num)
	typedef std::map< SinkKey, std::unique_ptr<ActionSink> >  ActionSinks;

	// typedef std::map< SinkKey, std::unique_ptr<EventSource> > EventSources;

	ActionSinks  actionSinks;

	std::vector<std::vector< std::unique_ptr<ActionSink> > > ECAchannels;
	std::vector< std::unique_ptr<ActionSink> >      *ECA_LINUX_channel; // a reference to the channels of type ECA_LINUX
	unsigned                                         ECA_LINUX_channel_index;
	unsigned                                         ECA_LINUX_channel_subchannels;

	uint16_t updateMostFull(unsigned channel); // returns current fill
	void resetMostFull(unsigned channel);
	void popMissingQueue(unsigned channel, unsigned num);


public:
	uint64_t ReadRawCurrentTime();
	
	void compile();
	etherbone::Device &get_device();

	EcaDriver(SAFTd *saftd, etherbone::Device &dev, const std::string &obj_path, saftbus::Container *cont);
	~EcaDriver();

	void setHandler(unsigned channel, bool enable, eb_address_t address);
	void msiHandler(eb_data_t msi, unsigned channel);
	void InjectEvent(uint64_t event, uint64_t param, eb_plugin::Time time);

	std::string NewSoftwareActionSink(const std::string& name);
	void removeSowftwareActionSink(SoftwareActionSink *sas);
	std::map< std::string, std::string > getSoftwareActionSinks() const;

	SoftwareActionSink *getSoftwareActionSink(const std::string & sas_obj_path);
};


} // namespace 

#endif

