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
#ifndef INPUT_H
#define INPUT_H

#include "InoutImpl.h"
#include "EventSource.h"
#include "interfaces/Input.h"
#include "interfaces/iInputEventSource.h"

namespace saftlib {

class Input : public EventSource, public iInputEventSource
{
  public:
    typedef Input_Service ServiceType;
    struct ConstructorType {
      Glib::ustring name;
      Glib::ustring objectPath;
      Glib::ustring partnerPath;
      TimingReceiver* dev;
      eb_address_t tlu;
      unsigned channel;
      Glib::RefPtr<InoutImpl> impl;
      sigc::slot<void> destroy;
    };

    static Glib::RefPtr<Input> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // Methods
    bool ReadInput();
    // Property getters
    guint32 getStableTime() const ;
    bool getInputTermination() const;
    bool getSpecialPurposeIn() const;
    bool getInputTerminationAvailable() const;
    bool getSpecialPurposeInAvailable() const;
    Glib::ustring getLogicLevelIn() const;
    Glib::ustring getTypeIn() const;
    Glib::ustring getOutput() const;
    // Property setters
    void setStableTime(guint32 val);
    void setInputTermination(bool val);
    void setSpecialPurposeIn(bool val);
    
    // Property signals
    //  sigc::signal< void, guint32 > StableTime;
    //  sigc::signal< void, bool > InputTermination;
    //  sigc::signal< void, bool > SpecialPurposeIn;
    
    // From iEventSource
    guint64 getResolution() const;
    guint32 getEventBits() const;
    bool getEventEnable() const;
    guint64 getEventPrefix() const;
    
    void setEventEnable(bool val);
    void setEventPrefix(guint64 val);
    //  sigc::signal< void, bool > EventEnable;
    //  sigc::signal< void, guint64 > EventPrefix;
    
  protected:
    Input(const ConstructorType& args);
    Glib::RefPtr<InoutImpl> impl;
    Glib::ustring partnerPath;
    eb_address_t tlu;
    unsigned channel;
    bool enable;
    guint64 event;
    guint32 stable;
    
    void configInput();
};

}

#endif /* INPUT_H */
