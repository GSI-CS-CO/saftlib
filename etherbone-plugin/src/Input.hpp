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
#ifndef EB_PLUGIN_INPUT_HPP_
#define EB_PLUGIN_INPUT_HPP_

#include "Io.hpp"
#include "ECA.hpp"
#include "ECA_TLU.hpp"
#include "EventSource.hpp"

namespace eb_plugin {

class Input : public EventSource
{
	public:

		Input(ECA &eca
			, ECA_TLU &eca_tlu
		    , const std::string &name
		    , const std::string &partnerPath
		    , unsigned channel
		    , unsigned num
		    , Io *io
		    , saftbus::Container *container = nullptr);

		// Methods
		bool ReadInput();
		// Property getters
		uint32_t getStableTime() const;
		uint32_t getIndexIn() const;
		bool getInputTermination() const;
		bool getSpecialPurposeIn() const;
		bool getGateIn() const;
		bool getInputTerminationAvailable() const;
		bool getSpecialPurposeInAvailable() const;
		std::string getLogicLevelIn() const;
		std::string getTypeIn() const;
		std::string getOutput() const;
		// Property setters
		void setStableTime(uint32_t val);
		void setInputTermination(bool val);
		void setSpecialPurposeIn(bool val);
		void setGateIn(bool val);

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
		ECA &eca;
		ECA_TLU &eca_tlu;
		Io *io;
		std::string partnerPath;
		eb_address_t tlu;
		unsigned channel;
		bool enable;
		uint64_t event;
		uint32_t stable;

		// void configInput();
};

}

#endif /* INPUT_H */
