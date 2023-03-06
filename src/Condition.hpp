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

#ifndef CONDITION_H
#define CONDITION_H

#include <cstdint>
#include <string>

namespace saftlib {

class ActionSink;

/// de.gsi.saftlib.Condition:
/// @brief A rule matched against incoming events
///
/// Conditions are created for ActionSinks to select which events the
/// sink should respond to. Different ActionSinks return handles to 
/// different Conditions. This interface is the common denominator.
class Condition  
{
  public:
    // if the created with active=true, you must manually run compile() on TimingReceiver
    Condition(ActionSink *sink, unsigned number, bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag);
    virtual ~Condition() = default; 
    

    /// @brief The event identifier which this condition matches against.
    /// @return The event identifier which this condition matches against.
    ///
    /// An incoming event matches if its ID and this ID agree on all the bits
    /// set in this condition's Mask.
    ///
    // @saftbus-export
    uint64_t getID() const;
    // @saftbus-export
    void setID(uint64_t val);

    /// @brief  The mask used when comparing event IDs.
    /// @return The mask used when comparing event IDs.
    ///
    /// An incoming event matches if its ID and the ID property of this
    /// Condition agree on all the bits set in this Mask.
    ///
    // @saftbus-export
    uint64_t getMask() const;
    // @saftbus-export
    void setMask(uint64_t val);


    /// @brief  Added to an event's time to calculate the action's time.
    /// @return Added to an event's time to calculate the action's time.
    ///
    // @saftbus-export
    int64_t getOffset() const;
    // @saftbus-export
    void setOffset(int64_t val);
    
    /// @brief  Should late actions be executed? Defaults to false -->
    /// @return Should late actions be executed? Defaults to false -->
    ///
    // @saftbus-export
    bool getAcceptLate() const;
    // @saftbus-export
    void setAcceptLate(bool val);

    /// @brief Should early actions be executed? Defaults to false
    /// @return Should early actions be executed? Defaults to false
    ///
    // @saftbus-export
    bool getAcceptEarly() const;
    // @saftbus-export
    void setAcceptEarly(bool val);

    /// @brief  Should conflicting actions be executed? Defaults to false
    /// @return Should conflicting actions be executed? Defaults to false
    ///
    // @saftbus-export
    bool getAcceptConflict() const;
    // @saftbus-export
    void setAcceptConflict(bool val);

    /// @brief  Should delayed actions be executed? Defaults to true
    /// @return Should delayed actions be executed? Defaults to true
    ///
    // @saftbus-export
    bool getAcceptDelayed() const;
    // @saftbus-export
    void setAcceptDelayed(bool val);

    /// @brief  The condition should be actively matched against events.
    /// @return The condition should be actively matched against events.
    ///
    /// An inactive condition is not used to match against events.
    /// You can toggle the active state of this condition via this property,
    /// or if multiple Conditions should be atomically adjusted, use the
    /// ToggleActive method on the ActionSink.    
    ///
    // @saftbus-export
    bool getActive() const;
    // @saftbus-export
    void setActive(bool val);
    

    std::string &getObjectPath() { return objectPath; }
    
    
    // used by TimingReceiver and ActionSink
    uint32_t getRawTag() const { return tag; }
    void setRawActive(bool val) { active = val; }
    

    unsigned getNumber() const { return number; } 
  protected:

    std::string objectPath;
    ActionSink* sink;
    unsigned number;
    
    uint64_t id;
    uint64_t mask;
    int64_t offset;
    uint32_t tag;
    bool acceptLate, acceptEarly, acceptConflict, acceptDelayed;
    bool active;
};

}

#endif
