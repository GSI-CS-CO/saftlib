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

#ifndef SAFTLIB_EMBEDDEDCPU_ACTION_SINK_HPP_
#define SAFTLIB_EMBEDDEDCPU_ACTION_SINK_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include "ActionSink.hpp"

namespace saftlib {

/// de.gsi.saftlib.EmbeddedCPUActionSink:
/// @brief An output through which EmbeddedCPU actions flow.
/// 
/// This inteface allows the generation of SCU timing events.
/// A EmbeddedCPUActionSink is also an ActionSink and Owned.
/// 
/// If two SoftwareConditions are created on the same SoftwareActionSink
/// which require simultaneous delivery of two Actions, then they will be
/// delivered in arbitrary order, both having the 'conflict' flag set.
class EmbeddedCPUActionSink : public ActionSink {
	public:
		EmbeddedCPUActionSink(ECA &eca
		               , const std::string &object_path
		               , const std::string &name
		               , unsigned channel
		               , saftbus::Container *container = nullptr);
			
		/// @brief Create a condition to match incoming events
		///
		/// @param active Should the condition be immediately active
		/// @param id     Event ID to match incoming event IDs against
		/// @param mask   Set of bits for which the event ID and id must agree
		/// @param offset Delay in nanoseconds between event and action
		/// @param tag    The  32-bit value to send on the SCUbus
		/// @param result Object path to the created SCUbusCondition
		/// 
		/// This method creates a new condition that matches events whose
		/// identifier lies in the range [id &amp; mask, id | ~mask].  The offset
		/// acts as a delay which is added to the event's execution timestamp
		/// to determine the timestamp when the matching condition fires its
		/// action.  The returned object path is a SCUBUSCondition object.
		///
		// @saftbus-export
		std::string NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag);


};

}

#endif
