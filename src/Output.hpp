/*  Copyright (C) 2011-2016, 2021-2023 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#ifndef saftlib_OUTPUT_HPP
#define saftlib_OUTPUT_HPP

#include "Io.hpp"
#include "ActionSink.hpp"

namespace saftlib {

/// de.gsi.saftlib.OutputActionSink
/// @brief An output through which on/off actions flow.
///
/// This inteface allows the generation of Output pulses.
/// An OutputActionSink is also an ActionSink and Owned.
///
/// If two SoftwareConditions are created on the same SoftwareActionSink
/// which require simultaneous delivery of two Actions, then they will be
/// delivered in arbitrary order, both having the 'conflict' flag set.
///
class Output : public ActionSink
{
	public:
		Output(ECA &eca
		     , Io &io
		     , const std::string &output_object_path
		     , const std::string &input_partner_path
		     , unsigned channel
		     , saftbus::Container *container = nullptr);

		/// @brief Create a condition to match incoming events
		///
		/// @param active Should the condition be immediately active
		/// @param id     Event ID to match incoming event IDs against
		/// @param mask   Set of bits for which the event ID and id must agree
		/// @param offset Delay in nanoseconds between event and action
		/// @param on     The output should be toggled on (or off)
		/// @param result Object path to the created OutputCondition
		///
		/// This method creates a new condition that matches events whose
		/// identifier lies in the range [id &amp; mask, id | ~mask].  The offset
		/// acts as a delay which is added to the event's execution timestamp to
		/// determine the timestamp when the matching condition fires its action.
		/// The returned object path is a OutputCondition object.
		///
		// @saftbus-export
		std::string NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, bool on);


		/// @brief Directly manipulate the output value.
		/// @param value true enables the output driver
		///
		/// Set the output to on/off. Overwrite the previous state,
		/// regardless of whether it came from WriteOutput or a matching
		/// Condition. Similarly, the value may in turn be overwritten by
		/// a subsequent matching Condition or WriteOutput.
		///
		// @saftbus-export
		void WriteOutput(bool value);


		/// @brief Read the output state.
		/// @return true if the output is enabled
		///
		/// This property reflects the current value which would be output when
		/// OutputEnable is true. This may differ from ReadInput on an inout.
		///
		// @saftbus-export
		bool ReadOutput();

		/// @brief Read the combined output state.
		/// @return the combined output value
		///
		/// Every output IO has multiple sources, one or more can be active at the same time. This property shows the real (combined) output 
		/// state.
		///
		// @saftbus-export
		bool ReadCombinedOutput();


		/// @brief Starts the clock generator with given parameters.
		/// @return true if the clock is running
		///
		/// All parameters expect the value in nanoseconds.
		///
		// @saftbus-export
		bool StartClock(double high_phase, double low_phase, uint64_t phase_offset);


		/// @brief Get the status of the clock generator
		/// @return true if the clock is running
		///
		/// All parameters are in nanoseconds.
		///
		// @saftbus-export
		bool ClockStatus(double &high_phase, double &low_phase, uint64_t &phase_offset);

		/// @brief Stops the clock generator.
		/// @return flase if the clock was stopped
		///
		// @saftbus-export
		bool StopClock();

		/// @brief IO index.
		/// @return IO index
		///
		// @saftbus-export
		uint32_t getIndexOut() const;
		
		/// @brief Is the output driver enabled.
		///
		/// When OutputEnable is false, the output is not driven.
		/// This defaults to off.
		/// See also Termination if this is an inoutput.
		///
		// @saftbus-export
  		void setOutputEnable(bool val);
		// @saftbus-export
		bool getOutputEnable() const;
		

		/// @brief Enable or disable the special function
		/// @param val true enables the special function
		///
		// @saftbus-export
		void setSpecialPurposeOut(bool val);
		// @saftbus-export
		bool getSpecialPurposeOut() const;
		
		/// @brief Set output gate or get gate status.
		/// @param val true enables the gate
		///
		// @saftbus-export
		void setGateOut(bool val);
		// @saftbus-export
		bool getGateOut() const;
		
		/// @brief Output BuTiS t0 with timestamp.
		/// @param val true enables BuTiS output
		///
		// @saftbus-export
		void setBuTiSMultiplexer(bool val);
		// @saftbus-export
		bool getBuTiSMultiplexer() const;
		

		/// @brief Output PPS signal from White Rabbit core.
		/// @param val true enables PPS signal
		///
		// @saftbus-export
		void setPPSMultiplexer(bool val);
		// @saftbus-export
		bool getPPSMultiplexer() const;

		/// @brief Can output enable be configured.
		/// @return true if Output can be enabled or disabled
		///
		// @saftbus-export
		bool getOutputEnableAvailable() const;
		
		/// @brief Can special configuration be configured.
		/// @return true if a special purpose function is available
		///
		// @saftbus-export
		bool getSpecialPurposeOutAvailable() const;
		
		/// @brief Logic level of the output 
		/// @return (LVDS, LVTTL, ...)
		///
		// @saftbus-export
		std::string getLogicLevelOut() const;

		/// @brief IO type 
		/// @return (GPIO, LVDS, ...)
		///
		// @saftbus-export
		std::string getTypeOut() const;

		/// @brief If non-empty, path of the Input object for the same physical IO
		/// @return object path of the Input object for the same physical IO, or an empty string
		///
		// @saftbus-export
		std::string getInput() const;



		// Property signals
		//   sigc::signal< void, bool > OutputEnable;
		//   sigc::signal< void, bool > SpecialPurposeOut;
		//   sigc::signal< void, bool > BuTiSMultiplexer;

	protected:
		Io &io;
		std::string partnerPath;

		double   clk_low_phase;
		double   clk_high_phase;
		uint64_t clk_phase_offset;
};

}

#endif 