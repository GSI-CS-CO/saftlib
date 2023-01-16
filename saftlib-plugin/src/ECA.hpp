/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
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

#ifndef saftlib_ECA_DRIVER_HPP_
#define saftlib_ECA_DRIVER_HPP_


#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <saftbus/service.hpp>

#include <memory>

namespace saftlib {

class SAFTd;
class OpenDevice;
class ActionSink;
class SoftwareActionSink;
class Output;
/// @brief ECA (Event Condition Action) 
/// ECA is a hardwar unit cabable of executing actions at a given time (1 ns resolution) in response to events that meet a condition.
/// An event contains a 64-bit timestamp, 64-bit id, 64-bit flags
/// A condition contains a 64-bit id, 64-bit prefix-mask, 64-bit time offset
/// Action can be one of the following:
///   - Output: change output driver (rising or falling edge on one of the GPIOs) 
///   - SoftwareActionSink: send the content of the event to host system (store the event content in some registers and send an MSI so that the host can read the event information)
///   - execute a preconfigured wishbone access
///   - SCUbusActionSink: write a tag on SCU bus
///   - ECPU : send content of the event to embedded CPU
///
/// Hardware supports are up to 32 SoftwareActionSink at the same time. The hardware has one single Output channel configured for the host system with 32 sub-channels for the different SoftwareActionSinks. 
/// Each SoftwareActionSinks occupies one of the sub-channels. This implementation uses an std::vector< std::unique_ptr< ActionSink> > which is resized in the constructor to the number of available sub-channels.
/// Each invalid unique_ptr<ActionSink> indicates an un-occupied sub-channel. Each SoftwareActionSink can have many conditions. On execution, ECA writes a tag value into the ActionSink that refers to the condition that matched the incoming event.
class ECA {
	friend class ActionSink;



	etherbone::Device  &device;
	std::string        object_path;
	saftbus::Container *container;
	uint64_t sas_count; // counts number of SoftwareActionSinks


	
	eb_address_t base;        // address of ECA control slave
	// eb_address_t stream;      // address of ECA event input slave
	eb_address_t first, last; // Msi address range
	eb_address_t msi_first, msi_last;


	unsigned channels;        // number of available ECA otput channels
	unsigned search_size;
	unsigned walker_size;
	unsigned max_conditions;
	unsigned used_conditions;
	std::vector<eb_address_t> channel_msis;
	std::vector<eb_address_t> queue_addresses;
	std::vector<uint16_t> most_full;

	// public type, even though the member is private
	// typedef std::map< SinkKey, std::unique_ptr<ActionSink> >  ActionSinks;
	// ActionSinks  actionSinks;

	std::vector<std::vector< std::unique_ptr<ActionSink> > > ECAchannels;
	std::vector< std::unique_ptr<ActionSink> >      *ECA_LINUX_channel; // a reference to the channels of type ECA_LINUX
	unsigned                                         ECA_LINUX_channel_index;
	unsigned                                         ECA_LINUX_channel_subchannels;

	std::map<std::string, std::string > scubus_action_sinks; // a list of scubus_action_sinks that is created on construction and returned by getSCUbusActionSinks()
	std::map<std::string, std::string > ecpu_action_sinks; // a list of ecpu_action_sinks that is created on construction and returned by getEmbeddedCPUActionSinks()
	std::map<std::string, std::string > wbm_action_sinks; 

	void popMissingQueue(unsigned channel, unsigned num);	
	void probeConfiguration();
	void prepareChannels();
	void setMsiHandlers(SAFTd &saftd);
	void msiHandler(eb_data_t msi, unsigned channel);
	void setHandler(unsigned channel, bool enable, eb_address_t address);

	// void compile();



	uint16_t getMostFull(int channel);	
	eb_address_t get_base_address();

public:
	const std::string &get_object_path();
	etherbone::Device &get_device();
	void compile();
	typedef std::pair<unsigned, unsigned> SinkKey; // (channel, num)


	ECA(SAFTd &saftd, etherbone::Device &device, const std::string &object_path, saftbus::Container *container);
	virtual ~ECA();


	/// @brief add sink and let ECA take ownership of the sink object
	bool addActionSink(int channel, std::unique_ptr<ActionSink> sink);

	uint16_t updateMostFull(unsigned channel); // returns current fill

	void removeSowftwareActionSink(SoftwareActionSink *sas);

	/// @brief The current time of the timingreceiver.
	/// @return     the current time of the timingreceiver
	///
	/// The result is the WhiteRabbit timesamps in nanoseconds.
	/// It is not checked if WhiteRabbit core is locked or not.
	///
	// @saftbus-export
	uint64_t ReadRawCurrentTime();
	
	/// @brief        Create a new SoftwareActionSink.
	/// @param name   A name for the SoftwareActionSink. Can be left blank.
	/// @return       Object path to the created SoftwareActionSink.
	///
	/// SoftwareActionSinks allow a program to create conditions that match
	/// incoming timing events.  These conditions may have callback methods
	/// attached to them in order to receive notification.  The returned
	/// path corresponds to a SoftwareActionSink that is owned by the
	/// process which claimed it, and can thus be certain that no other
	/// processes can interfere with the results.
	///
	// @saftbus-export	
	std::string NewSoftwareActionSink(const std::string& name);

	/// @brief A list of all current SoftwareActionSinks.
	/// @return A list of all current SoftwareActionSinks.
	///
	/// Typically, these SoftwareActionSinks will be owned by their
	/// processes and not of much interest to others.  Therefore, many of
	/// the entries here may be of no interest to a particular client. 
	/// However, it is possible for a SoftwareActionSink to be Disowned, in
	/// which case it may be persistent and shared between programs under a
	/// well known name.
	///
	// @saftbus-export
	std::map< std::string, std::string > getSoftwareActionSinks() const;

	/// @brief A list of all SCUbusActionSinks
	/// @return A list of all SCUbusActionSinks
	///
	// @saftbus-export
	std::map< std::string, std::string > getSCUbusActionSinks() const;

	/// @brief A list of all EmbeddedCPUActionSinks
	/// @return A list of all EmbeddedCPUActionSinks
	///
	// @saftbus-export
	std::map< std::string, std::string > getEmbeddedCPUActionSinks() const;

	/// @brief A list of all WbmActionSinks
	/// @return A list of all WbmActionSinks
	///
	// @saftbus-export
	std::map< std::string, std::string > getWbmActionSinks() const;

	/// @brief get a pointer to a SoftwareActionSink in a stand-alone application
	/// @param sas_obj_path Object path of the SoftwareActionSink
	/// @return pointer to a SoftwareActionSink 
	///
	SoftwareActionSink *getSoftwareActionSink(const std::string & sas_obj_path);

	/// @brief A list of all the high/low outputs on the receiver.
	/// @return A list of all the high/low outputs on the receiver.
	///
	/// Each path refers to an object of type Output.	
	///
	// @saftbus-export
	std::map< std::string, std::string > getOutputs() const;

	/// @brief The number of additional conditions that may be activated.
	///
	/// The ECA has limited hardware resources in its match table.
	/// @return number of additional conditions that may be activated.
	// @saftbus-export
	uint32_t getFree() const;

	void resetMostFull(unsigned channel);


	Output *getOutput(const std::string &output_obj_path);
};


} // namespace 

#endif

