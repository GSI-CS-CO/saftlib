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
#ifndef EB_PLUGIN_TIMING_RECEIVER_HPP_
#define EB_PLUGIN_TIMING_RECEIVER_HPP_

#include <deque>
#include <memory>
#include <string>

#include <saftbus/service.hpp>

#include <sys/stat.h>

#include "eb-source.hpp"

#include "ActionSink.hpp"

namespace eb_plugin {

class SAFTd;
class TimingReceiver {
public:
	TimingReceiver(SAFTd *saftd, etherbone::Socket &socket, const std::string &object_path, const std::string &name, const std::string etherbone_path, saftbus::Container *container = nullptr);
	~TimingReceiver();

	const std::string &get_object_path() const;

	// @saftbus-export
	void Remove();
	// @saftbus-export
	std::string getEtherbonePath() const;
	// @saftbus-export
	std::string getName() const;



	// @saftbus-export
	std::string NewSoftwareActionSink(const std::string& name);
	// // @saftbus-export
	// void InjectEvent(uint64_t event, uint64_t param, uint64_t time);
	// // @saftbus-export
	// void InjectEvent(uint64_t event, uint64_t param, saftlib::Time time);
	// // @saftbus-export
	// uint64_t ReadCurrentTime();
	// // @saftbus-export
	// saftlib::Time CurrentTime();
	// @saftbus-export
	std::map< std::string, std::string > getGatewareInfo() const;
	// // @saftbus-export
	// std::string getGatewareVersion() const;
	
	// @saftbus-export
	bool getLocked() const;
	
	// // @saftbus-export
	// bool getTemperatureSensorAvail() const;
	// // @saftbus-export
	// int32_t CurrentTemperature();
	// @saftbus-export
	std::map< std::string, std::string > getSoftwareActionSinks() const;
	// // @saftbus-export
	// std::map< std::string, std::string > getOutputs() const;
	// // @saftbus-export
	// std::map< std::string, std::string > getInputs() const;
	// // @saftbus-export
	// std::map< std::string, std::map< std::string, std::string > > getInterfaces() const;
	// // @saftbus-export
	// uint32_t getFree() const;

	// @saftbus-signal
	std::function<void(bool locked)> SigLocked;


	// Compile the condition table
	void compile();


    // public type, even though the member is private
    typedef std::pair<unsigned, unsigned> SinkKey; // (channel, num)
    typedef std::map< SinkKey, std::unique_ptr<ActionSink> >  ActionSinks;
    // typedef std::map< SinkKey, std::unique_ptr<EventSource> > EventSources;


    etherbone::Device& getDevice() { return device; }
    eb_address_t getBase() { return base; }

private:
	bool aquire_watchdog(); 
    eb_data_t watchdog_value;

    void setupGatewareInfo(uint32_t address);
    std::map<std::string, std::string> gateware_info;

    bool poll();
    saftbus::Source *poll_timeout_source;


    unsigned channels;
    unsigned search_size;
    unsigned walker_size;
    unsigned max_conditions;
    unsigned used_conditions;
    std::vector<eb_address_t> channel_msis;
    std::vector<eb_address_t> queue_addresses;
    std::vector<uint16_t> most_full;

    uint64_t sas_count; // number of SoftwareActionSinks
    ActionSinks  actionSinks;
    // EventSources eventSources;
    // OtherStuff   otherStuff;
    void popMissingQueue(unsigned channel, unsigned num);


	mutable etherbone::Device device;
    
    eb_address_t stream;
    eb_address_t watchdog;
    eb_address_t pps;
    eb_address_t ats;
    eb_address_t info;

	SAFTd              *saftd; // need a pointer to SAFTd because ther MSI callbacks can be registered

	std::string object_path;
	std::string name;
	std::string etherbone_path;

	saftbus::Container *container; // need a pointer to container to register new Service objects (ActionSink, Condition, ...)

	mutable bool locked;

	struct stat dev_stat;

	eb_address_t first, last; // Msi address range

	eb_address_t base,  mask;

	  bool activate_msi_polling;
	  unsigned polling_interval_ms;

	friend class ActionSink;

};

} // namespace 

#endif
