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

namespace eb_plugin {

class SAFTd;
class TimingReceiver {
public:
	TimingReceiver(saftbus::Container *container, SAFTd *saftd, etherbone::Socket &socket, const std::string &object_path, const std::string &name, const std::string etherbone_path);
	~TimingReceiver();

	const std::string &get_object_path() const;

	// @saftbus-export
	void Remove();
	// @saftbus-export
	std::string getEtherbonePath() const;
	// @saftbus-export
	std::string getName() const;



	// // @saftbus-export
	// std::string NewSoftwareActionSink(const std::string& name);
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
	// // @saftbus-export
	// std::map< std::string, std::string > getSoftwareActionSinks() const;
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


private:
	bool aquire_watchdog(); 
    eb_data_t watchdog_value;

    void setupGatewareInfo(uint32_t address);
    std::map<std::string, std::string> gateware_info;

    bool poll();
    saftbus::Source *poll_timeout_source;

	mutable etherbone::Device device;
    
    eb_address_t stream;
    eb_address_t watchdog;
    eb_address_t pps;
    eb_address_t ats;
    eb_address_t info;

	saftbus::Container *container; // need a pointer to container to register new Service objects (ActionSink, Condition, ...)
	SAFTd              *saftd; // need a pointer to SAFTd because ther MSI callbacks can be registered

	std::string object_path;
	std::string name;
	std::string etherbone_path;


	mutable bool locked;

	struct stat dev_stat;

	eb_address_t first, last; // Msi address range

	eb_address_t base,  mask;

	//   typedef std::map<eb_address_t, sigc::slot<void, eb_data_t> > irqMap;
	//   static irqMap irqs;

	//   struct MSI { eb_address_t address, data; };
	//   typedef boost::circular_buffer<MSI> msiQueue;
	//   static msiQueue msis;

	  bool activate_msi_polling;
	  unsigned polling_interval_ms;
	//   bool poll_msi();
	// eb_address_t msi_first;

	// friend class EbSlaveHandler;
	// friend class MSI_Source;
};

} // namespace 

#endif
