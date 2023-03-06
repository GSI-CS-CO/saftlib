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

#ifndef SCUBUS_CONDITION_HPP_
#define SCUBUS_CONDITION_HPP_

#include <saftbus/service.hpp>

#include "Owned.hpp"
#include "Condition.hpp"
#include "ActionSink.hpp"

namespace saftlib {


class SCUbusCondition_Service;


/// de.gsi.saftlib.SCUbusCondition:
/// @brief: Matched against incoming events on a SCUbusActionSink.
/// 
/// SCUbusConditions are created by SCUbusActionSinks to select which
/// events should generate callbacks. This interface always implies
/// that the object also implements the general Condition interface.
class SCUbusCondition : public Owned, public Condition {
public:
	SCUbusCondition(ActionSink *sink, unsigned number, bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, saftbus::Container *container = nullptr);

	/// @brief The tag which is sent to the SCUbus by this condition
	/// @return The tag value
	// @saftbus-export
	uint32_t getTag() const;
	/// @brief The tag which is sent to the SCUbus by this condition
	/// @param val The tag value
	// @saftbus-export
	void setTag(uint32_t val);

	typedef SCUbusCondition_Service ServiceType;
};

}

#endif
