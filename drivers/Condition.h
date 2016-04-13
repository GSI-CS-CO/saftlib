/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
      Glib::ustring objectPath;
      ActionSink* sink;
      bool active;
      guint64 id;
      guint64 mask;
      gint64 offset;
      guint32 tag;
      sigc::slot<void> destroy;
    };
    Condition(const Condition_ConstructorType& args);
    // ~Condition(); // unnecessary; ActionSink::removeCondition executes compile()
    
    // iCondition
    guint64 getID() const;
    guint64 getMask() const;
    gint64 getOffset() const;
    
    bool getAcceptLate() const;
    bool getAcceptEarly() const;
    bool getAcceptConflict() const;
    bool getAcceptDelayed() const;
    bool getActive() const;
    
    void setID(guint64 val);
    void setMask(guint64 val);
    void setOffset(gint64 val);
    
    void setAcceptLate(bool val);
    void setAcceptEarly(bool val);
    void setAcceptConflict(bool val);
    void setAcceptDelayed(bool val);
    void setActive(bool val);
    
    // used by TimingReceiver and ActionSink
    guint32 getRawTag() const { return tag; }
    void setRawActive(bool val) { active = val; }
    
  protected:
    ActionSink* sink;
    guint64 id;
    guint64 mask;
    gint64 offset;
    guint32 tag;
    bool acceptLate, acceptEarly, acceptConflict, acceptDelayed;
    bool active;
};

}

#endif
