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
#ifndef EB_PLUGIN_SOFTWARE_ACTION_SINK_H
#define EB_PLUGIN_SOFTWARE_ACTION_SINK_H

#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#include <etherbone.h>

#include "ActionSink.hpp"

namespace eb_plugin {

	/// de.gsi.saftlib.SoftwareActionSink:
	/// @brief An output through which software actions flow.
	///
	/// A SoftwareActionSink guarantees ordered execution of all callbacks
	/// on SoftwareConditions created via the NewCondition method. Each
	/// SoftwareActionSink is independent of all others, so a single 
	/// program may operate in isolation without concern about potential
	/// conflicting rules from other clients on the same machine.
	///
	/// A SoftwareActionSink is both an ActionSink and Owned.
	///
	/// If two SoftwareConditions are created on the same SoftwareActionSink
	/// which require simultaneous delivery of two Actions, then they will be
	/// delivered in arbitrary order, both having the 'conflict' flag set.
	class SoftwareActionSink : public ActionSink {
	public:
		//SoftwareActionSink(const std::string &object_path, TimingReceiver *dev, const std::string &name, unsigned channel, unsigned num, eb_address_t queue, saftbus::Container *container = nullptr);
		SoftwareActionSink(TimingReceiver *dev
			             , const std::string &name
			             , unsigned channel, unsigned num, eb_address_t queue_address
			             , saftbus::Container *container = nullptr);

		/// NewCondition: Create a condition to match incoming events
		///
		///   @param active Should the condition be immediately active
		///   @param id     Event ID to match incoming event IDs against
		///   @param mask   Set of bits for which the event ID and id must agree
		///   @param offset Delay in nanoseconds between event and action
		///   @return       Object path to the created SoftwareCondition
		///
		/// This method creates a new condition that matches events whose
		/// identifier lies in the range [id & mask, id | ~mask].  The offset
		/// acts as a delay which is added to the event's execution timestamp
		/// to determine the timestamp when the matching condition fires its
		/// action.  The returned object path is a SoftwareCondition object.		
		///
		// @saftbus-export
		std::string NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset);

		// override receiveMSI to also pop the software queue
		void receiveMSI(uint8_t code);
		
	protected:
		eb_address_t queue;
	};

}

#endif
