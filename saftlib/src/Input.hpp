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

#ifndef saftlib_INPUT_HPP_
#define saftlib_INPUT_HPP_

#include "Io.hpp"
#include "ECA.hpp"
#include "ECA_TLU.hpp"
#include "EventSource.hpp"

namespace saftlib {

class Input : public EventSource
{
	public:

		Input(ECA_TLU &eca_tlu
		    , const std::string &input_object_path
			, const std::string &output_partner_path
		    , unsigned eca_tlu_chan
		    , Io *io
		    , saftbus::Container *container = nullptr);

		// Methods
		/// @brief Read the current input value.
		/// @return The current logic level on the input.
		///
		/// For inoutputs, this may differ from the Output value, if OutputEnable
		/// is false.  To receive a signal on Input changes, use the EventSource
		/// interface to create timing events and monitor these via a
		/// SoftwareActionSink.
		// @saftbus-export
		bool ReadInput();
		// Property getters

		/// @brief Deglitch threshold for the input
		/// @return number of nanoseconds input signal needs to be stable before an edge is detected
		///
		/// The number of nanoseconds a signal must remain high or low in order
		/// to be considered a valid transition. Increasing this value will not
		/// impact the resulting timestamps, but will hide transitions smaller
		/// than the threshold. For example, if StableTime=400, then a 5MHz
		/// signal would be completely ignored.		
		// @saftbus-export
		uint32_t getStableTime() const;
		/// @brief Deglitch threshold for the input
		/// @param val number of nanoseconds the input needs to be stable after an edge
		// @saftbus-export
		void setStableTime(uint32_t val);

		/// @brief Each IO on hardware has a unique index 
		/// @return IO index
		// @saftbus-export
		uint32_t getIndexIn() const;

		/// @brief Is the special function enabled.
		/// @return true if special function is enabled, false otherwise.
		// @saftbus-export
		bool getSpecialPurposeIn() const;
		/// @brief enable or disable special function
		// @saftbus-export
		void setSpecialPurposeIn(bool val);
		/// @brief Some inputs have special purpose configuration. What exacly depends on the input.
		/// @return true if input has special configuration.
		// @saftbus-export
		bool getSpecialPurposeInAvailable() const;

		/// @brief Inputs have a logic gate to prevent inputs to propagate further into the system (ECA_TLU)
		/// @return true if gate is open
		// @saftbus-export
		bool getGateIn() const;
		/// @brief Inputs have a logic gate to prevent inputs to propagate further into the system (ECA_TLU)
		/// @param val true enables the gate, false disables it
		// @saftbus-export
		void setGateIn(bool val);

		
		/// @brief 50 ohm input termination
		/// @return true if termination is enabled, false otherwise.
		///
		/// Some inputs need termination to receive a clean input signal.
		/// However, if the same IO is used as an Output, termination should
		/// probably be disabled.  This defaults to on.  See also OutputEnable if
		/// this is an inoutput.
		// @saftbus-export
		bool getInputTermination() const;
		/// @brief enable or disable 50 ohm input termination
		/// @param val true enables input termination
		// @saftbus-export
		void setInputTermination(bool val);
		/// @brief Inputs can have a configurable 50 ohm termination.
		/// @return true if (input) termination be configured. false otherwise.
		// @saftbus-export
		bool getInputTerminationAvailable() const;
		
		
		/// @brief Inputs support different logic levels (LVDS, LVTTL, ...)
		/// @return Logic level of the input (LVDS, LVTTL, ...)
		// @saftbus-export
		std::string getLogicLevelIn() const;
		
		/// @brief IO type (GPIO, LVDS, ...)
		/// @return the IO type
		// @saftbus-export
		std::string getTypeIn() const;
		
		/// @brief path of the Output object for the same physical IO
		/// @return Empty string if this is not an output. Path of the Output object for the same physical IO otherwise.
		// @saftbus-export
		std::string getOutput() const;
		// Property setters

		// From iEventSource
		uint64_t getResolution() const;
		uint32_t getEventBits() const;
		bool getEventEnable() const;
		uint64_t getEventPrefix() const;

		void setEventEnable(bool val);
		void setEventPrefix(uint64_t val);
		//  sigc::signal< void, bool > EventEnable;
		//  sigc::signal< void, uint64_t > EventPrefix;

	private:
		ECA_TLU &eca_tlu;
		Io *io;
		std::string partnerPath;
		eb_address_t tlu;
		unsigned eca_tlu_channel;
		bool enable;
		uint64_t event;
		uint32_t stable;

		// void configInput();
};

}

#endif /* INPUT_H */
