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
class SoftwareActionSink;

class WatchdogDriver;
class PpsDriver;
class EcaDriver;



/// de.gsi.saftlib.TimingReceiver:
/// @brief A timing receiver.
/// @param saftd A pointer to a SAFTd object. The SAFTd object holds the etherbone socket
///              on which the MSI arrive.
/// @param name The logical name of the Timing receiver
/// @param etherbone_path The etherbone path of the hardware
/// @param container A pointer to a saftbus container. If an instance of TimingReceiver 
///                  should run on a saftbus deamon, container must point to the saftbus::Container 
///                  instance. If the TimingReceiver is used stand-alone, this pointer must be nullptr.
///
/// Timing receivers are attached to saftlib by specifying a name and an etherbone path.  The
/// name should denote the logical relationship of the device to saftd. 
/// For example, baseboard would be a good name for the timing receiver
/// attached to an SCU.  If an exploder is being used to output events to
/// an oscilloscope, a good logical name might be scope.  In these
/// examples, the path for the SCU baseboard would be dev/wbm0, and the
/// scope exploder would be dev/ttyUSB3 or similar.
/// This scheme is intended to make it easy to hot-swap hardware. If the
/// exploder dies, you can simply attach a new one under the same logical
/// name, even though the path might be different.
///
/// Timing receivers can respond to timing events from the data master.
/// The can also respond to external timing triggers via inputs.
/// The general idea is that a TimingReceiver has ActionSinks to which it
/// sends actions in response to incoming timing events. Timing events
/// are matched with Conditions to create the Actions sent to the Sinks.
/// EventSources are objects which create timing events, to be matched
/// by the conditions. The data master is a global EventSource to which
/// all TimingReceivers listen. However, external inputs can also be
/// configured to generate timing events. Furthermore, a TimingReceiver
/// can simulate the receipt of a timing event by calling the InjectEvent
/// method.
///
/// Timing receivers always typically have binary outputs lines
/// (OutputActionSinks), which are listed in the Outputs property. 
/// Similarly, they often have digital inputs (InputEventSources).
/// Some timing receivers have special purpose interfaces. For example,
/// an SCU has the SCUbusActionSink which generates 32-bit messages over
/// the SCU backplane. These special interfaces can be found in the
/// interfaces property. The SCU backplane would be found under the
/// SCUbusActionSink key, and as there is only one, it would be the 0th.
///
class TimingReceiver {
public:
	TimingReceiver(SAFTd *saftd, const std::string &name, const std::string etherbone_path, 
		           saftbus::Container *container = nullptr);
	~TimingReceiver();

	const std::string &get_object_path() const;

	/// @brief Remove the device from saftlib management.
	///
	// @saftbus-export
	void Remove();

	/// @brief The path through which the device is reached.
	/// @return The path through which the device is reached.
	///
	// @saftbus-export
	std::string getEtherbonePath() const;

	/// @brief The logical name with which the device was connected.
	/// @return The logical name with which the device was connected.
	///
	// @saftbus-export
	std::string getName() const;


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


	/// @brief The method is depricated, use InjectEvent with saftlib::Time instead.
	/// @param event The event identifier which is matched against Conditions
	/// @param param The parameter field, whose meaning depends on the event ID.
	/// @param time  The execution time for the event, added to condition offsets.
	///
	/// Simulate the receipt of a timing event
	/// Sometimes it is useful to simulate the receipt of a timing event. 
	/// This allows software to test that configured conditions lead to the
	/// desired behaviour without needing the data master to send anything.
	///
	// @saftbus-export
	void InjectEvent(uint64_t event, uint64_t param, uint64_t time);


	/// @brief        Simulate the receipt of a timing event
	/// @param event  The event identifier which is matched against Conditions
	/// @param param  The parameter field, whose meaning depends on the event ID.
	/// @param time   The execution time for the event, added to condition offsets.
	///
	/// Sometimes it is useful to simulate the receipt of a timing event. 
	/// This allows software to test that configured conditions lead to the
	/// desired behaviour without needing the data master to send anything.
	///
	// @saftbus-export
	void InjectEvent(uint64_t event, uint64_t param, eb_plugin::Time time);

	/// @brief This method is deprecaded, use CurrentTime instead. 
	/// @return         Nanoseconds since 1970.
	///
	/// The current time in nanoseconds since 1970.
	/// Due to delays in software, the returned value is probably several
	/// milliseconds behind the true time.
	///
	// // @saftbus-export
	// uint64_t ReadCurrentTime();


	/// @brief The current time of the timingreceiver.
	/// @return     the current time of the timingreceiver
	///
	/// The result type is saftlib::Time, which can be used to obtain 
	/// either the number of nanoseconds since 1970, or the same value
	/// minus the current UTC offset.
	/// Due to delays in software, the returned value is probably several
	/// milliseconds behind the true time.
	///
	// @saftbus-export
	eb_plugin::Time CurrentTime();

	/// @brief Key-value map of hardware build information
	/// @return Key-value map of hardware build information
	///
	// @saftbus-export
	std::map< std::string, std::string > getGatewareInfo() const;

	/// @brief Hardware build version
	/// @return "major.minor.tiny" if version is valid (or "N/A" if not available)
	///
	// @saftbus-export
	std::string getGatewareVersion() const;
	
	/// @brief The timing receiver is locked to the timing grandmaster.
	/// @return The timing receiver is locked to the timing grandmaster.
	///
	/// Upon power-up it takes approximately one minute until the timing
	/// receiver has a correct timestamp.
	///
	// @saftbus-export
	bool getLocked() const;

	/// @brief This signal is sent when the Lock Property changes
	///
	// @saftbus-signal
	std::function<void(bool locked)> SigLocked;
	

	/// @brief Check if a temperature sensor is available
	/// @return Check if a temperature sensor is available
	///
	/// in a timing receiver.
	///
	// // @saftbus-export
	// bool getTemperatureSensorAvail() const;

	/// @brief The current temperature in degree Celsius.
	/// @return         Temperature in degree Celsius.
	///
	/// The valid temperature range is from -70 to 127 degree Celsius.
	/// The data type is 32-bit signed integer.
	///
	// // @saftbus-export
	// int32_t CurrentTemperature();


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

	/// @brief A list of all the high/low outputs on the receiver.
	/// @return A list of all the high/low outputs on the receiver.
	///
	/// Each path refers to an object of type Output.	
	///
	// // @saftbus-export
	// std::map< std::string, std::string > getOutputs() const;

	/// @brief A list of all the high/low inputs on the receiver.
	/// @return  A list of all the high/low inputs on the receiver.
	///
	/// Each path refers to an object of type Input.	
	///
	// // @saftbus-export
	// std::map< std::string, std::string > getInputs() const;

	/// @brief List of all object instances of various hardware.
	/// @return List of all object instances of various hardware.
	///
	/// The key in the dictionary is the name of the interface.
	/// The value is all object paths to hardware implementing that interface.	
	///
	// // @saftbus-export
	// std::map< std::string, std::map< std::string, std::string > > getInterfaces() const;

	/// @brief The number of additional conditions that may be activated.
	/// @return The number of additional conditions that may be activated.
	///
	/// The ECA has limited hardware resources in its match table.	
	///
	// // @saftbus-export
	// uint32_t getFree() const;



	// Compile the condition table
	void compile();


	// public type, even though the member is private
	// typedef std::pair<unsigned, unsigned> SinkKey; // (channel, num)
	// typedef std::map< SinkKey, std::unique_ptr<ActionSink> >  ActionSinks;

	// typedef std::map< SinkKey, std::unique_ptr<EventSource> > EventSources;


	etherbone::Device& getDevice() { return device; }
	eb_address_t getBase() { return base; }

	SoftwareActionSink *getSoftwareActionSink(const std::string & object_path);

	void removeSowftwareActionSink(SoftwareActionSink *sas);

	uint64_t ReadRawCurrentTime();

private:


	void setupGatewareInfo(uint32_t address);
	std::map<std::string, std::string> gateware_info;

	bool poll();
	saftbus::Source *poll_timeout_source;


	mutable etherbone::Device device;

	std::unique_ptr<WatchdogDriver> watchdog;
	std::unique_ptr<PpsDriver>      pps;
	std::unique_ptr<EcaDriver>      eca;
	
	eb_address_t stream;
	eb_address_t ats;
	eb_address_t info;

	eb_address_t mbox_for_testing_only; // only here to see if msis from software-tr work

	SAFTd              *saftd; // need a pointer to SAFTd because ther MSI callbacks can be registered

	std::string object_path;
	std::string name;
	std::string etherbone_path;

	saftbus::Container *container; // need a pointer to container to register new Service objects (ActionSink, Condition, ...)

	struct stat dev_stat;

	eb_address_t base,  mask;

	  bool activate_msi_polling;
	  unsigned polling_interval_ms;

	friend class ActionSink;

};




class EcaDriver {
	//friend class TimingReceiver;
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
