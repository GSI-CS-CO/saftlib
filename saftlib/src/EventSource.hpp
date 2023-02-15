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

#ifndef saftlib_EVENTSOURCE_HPP_
#define saftlib_EVENTSOURCE_HPP_

#include "Owned.hpp"

namespace saftlib {

class TimingReceiver;

class EventSource : public Owned {
    std::string object_path;
    std::string object_name;
public:
	EventSource(const std::string &object_path, const std::string &name, saftbus::Container *container);

	/// @brief The precision of generated timestamps in nanoseconds.
	/// @return The precision of generated timestamps in nanoseconds.
	///
	// @saftbus-export
    virtual uint64_t getResolution() const = 0;

	/// @brief How many bits of external data are included in the ID
	/// @return How many bits of external data are included in the ID
	///
	// @saftbus-export
    virtual uint32_t getEventBits() const = 0;


    /// @brief Should the event source generate events
    /// @return true if the event source generates events
    ///
    // @saftbus-export
    virtual bool getEventEnable() const = 0;
    // @saftbus-export
    virtual void setEventEnable(bool val) = 0;

    /// @brief Combined with low EventBits to create generated IDs
    /// @return the event prefix
    ///
    // @saftbus-export
    virtual uint64_t getEventPrefix() const = 0;
    // @saftbus-export
    virtual void setEventPrefix(uint64_t val) = 0;

    std::string getObjectPath() const;
    std::string getObjectName() const;
};

}

#endif
