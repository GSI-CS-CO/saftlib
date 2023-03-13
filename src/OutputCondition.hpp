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

#ifndef saftlib_OUTPUT_CONDITION_HPP
#define saftlib_OUTPUT_CONDITION_HPP

#include "Owned.hpp"
#include "Condition.hpp"

// @saftbus-include
#include <Time.hpp>

#include <saftbus/service.hpp>

#include <functional>

namespace saftlib {

class OutputCondition_Service;

class OutputCondition : public Condition 
{
public:
	OutputCondition(ActionSink *sink, unsigned number, bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, saftbus::Container *container);

    /// de.gsi.saftlib.OutputCondition:
    /// @brief: Matched against incoming events on an OutputActionSink.
    /// 
    /// OutputConditions are created by OutputActionSinks to select which
    /// events should generate signal toggles. This interface always implies
    /// that the object also implements the general Condition interface.
    // @saftbus-export
    bool getOn() const;
    // @saftbus-export
    void setOn(bool val);

    // this typedef is needed for the ActionSink::NewCondition template function
    typedef OutputCondition_Service ServiceType;
};

}

#endif
