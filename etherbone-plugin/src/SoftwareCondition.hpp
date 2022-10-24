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
#ifndef SOFTWARE_CONDITION_H
#define SOFTWARE_CONDITION_H

#include "Owned.hpp"
#include "Condition.hpp"


// @saftbus-include
#include <Time.hpp>

#include <saftbus/service.hpp>

#include <functional>

namespace eb_plugin {


/// de.gsi.saftlib.SoftwareCondition
/// @brief Matched against incoming events on a SoftwareActionSink.
///
/// SoftwareConditions are created by SoftwareActionSinks to select which
/// events should generate callbacks. This interface always implies
/// that the object also implements the general Condition interface.
class SoftwareCondition : public Owned, public Condition
{
public:
	SoftwareCondition(const std::string &objectPath, ActionSink *sink, bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, saftbus::Container *container);

	/// @brief    Emitted whenever the condition matches a timing event.
	/// 
	/// @param event    The event identifier that matched this rule.
	/// @param param    The parameter field, whose meaning depends on the event ID.
	/// @param deadline The scheduled execution timestamp of the action (event time + offset).
	/// @param executed The actual execution timestamp of the action.
	/// @param flags    Whether the action was (ok=0,late=1,early=2,conflict=4,delayed=8)
	/// 
	/// While the underlying hardware strives to deliver the action precisely
	/// on the deadline, the software stack adds non-deterministic delay, so
	/// the deadline may be milliseconds in the past.  The late flag
	/// only indicates that the hardware failed to meet the required
	/// deadline. Similarly, the executed timestamp is the time when the
	/// hardware delivered the action, not when software has received it.
	/// Actions with error flags (late, early, conflict, delayed) are only
	/// delivered to this signal if the Condition which generated them
	/// specified that the respective error should be accepted.
	/// 
	// @saftbus-signal
	std::function< void(uint64_t event, uint64_t param, eb_plugin::Time deadline, eb_plugin::Time executed, uint16_t flags) > SigAction;

};

}

#endif