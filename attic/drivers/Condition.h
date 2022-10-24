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
#ifndef CONDITION_H
#define CONDITION_H

#include "interfaces/iCondition.h"
#include "Owned.h"

namespace saftlib {

class ActionSink;

class Condition : public Owned, public iCondition
{
  public:
    // if the created with active=true, you must manually run compile() on TimingReceiver
    struct Condition_ConstructorType {
      std::string objectPath;
      ActionSink* sink;
      bool active;
      uint64_t id;
      uint64_t mask;
      int64_t offset;
      uint32_t tag;
      sigc::slot<void> destroy;
    };
    Condition(const Condition_ConstructorType& args);
    // ~Condition(); // unnecessary; ActionSink::removeCondition executes compile()
    
    // iCondition
    uint64_t getID() const;
    uint64_t getMask() const;
    int64_t getOffset() const;
    
    bool getAcceptLate() const;
    bool getAcceptEarly() const;
    bool getAcceptConflict() const;
    bool getAcceptDelayed() const;
    bool getActive() const;
    
    void setID(uint64_t val);
    void setMask(uint64_t val);
    void setOffset(int64_t val);
    
    void setAcceptLate(bool val);
    void setAcceptEarly(bool val);
    void setAcceptConflict(bool val);
    void setAcceptDelayed(bool val);
    void setActive(bool val);
    
    // used by TimingReceiver and ActionSink
    uint32_t getRawTag() const { return tag; }
    void setRawActive(bool val) { active = val; }
    
  protected:
    ActionSink* sink;
    uint64_t id;
    uint64_t mask;
    int64_t offset;
    uint32_t tag;
    bool acceptLate, acceptEarly, acceptConflict, acceptDelayed;
    bool active;
};

}

#endif
