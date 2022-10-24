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

#include <saftbus/loop.hpp>

#include "OpenDevice.hpp"
#include "WhiteRabbit.hpp"
#include "Watchdog.hpp"
#include "ECA.hpp"
#include "ECA_TLU.hpp"
#include "BuildIdRom.hpp"
#include "TempSensor.hpp"
#include "IoControl.hpp"

// @saftbus-include
#include <Time.hpp>

namespace eb_plugin {

class SAFTd;

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
class TimingReceiver : public OpenDevice
                     , public WhiteRabbit
                     , public Watchdog
                     , public ECA
                     , public ECA_TLU
                     , public BuildIdRom
                     , public TempSensor {
public:
	TimingReceiver(SAFTd &saftd, const std::string &name, const std::string &etherbone_path, 
		           int polling_interval_ms = 1, saftbus::Container *container = nullptr);
	~TimingReceiver();

	const std::string &get_object_path() const;

	/// @brief Remove the device from saftlib management.
	///
	// @saftbus-export
	void Remove();

	/// @brief The logical name with which the device was connected.
	/// @return The logical name with which the device was connected.
	///
	// @saftbus-export
	std::string getName() const;

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



	/// @brief This signal is sent when the Lock Property changes
	///
	// @saftbus-signal
	std::function<void(bool locked)> SigLocked;
	


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
	// // @saftbus-export
	// std::map< std::string, std::string > getSoftwareActionSinks() const;

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



	// etherbone::Device& getDevice() { return OpenDevice::device; }

//	SoftwareActionSink *getSoftwareActionSink(const std::string & object_path);


private:

	IoControl io_control;


	bool poll();
	saftbus::Source *poll_timeout_source;

	
	eb_address_t ats;

	std::string object_path;
	std::string name;



	bool activate_msi_polling;
	unsigned polling_interval_ms;


};

} // namespace

#endif