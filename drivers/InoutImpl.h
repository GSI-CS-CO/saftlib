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
#ifndef INOUTPUT_IMPL_H
#define INOUTPUT_IMPL_H

#include <etherbone.h>
#include "TimingReceiver.h"

namespace saftlib {

class InoutImpl : public Glib::Object
{
  public:
    struct ConstructorType {
      TimingReceiver* tr;
      unsigned io_channel;
      unsigned io_index;
      unsigned io_special_purpose;
      unsigned io_logic_level;
      bool io_oe_available;
      bool io_term_available;
      bool io_spec_out_available;
      bool io_spec_in_available;
      eb_address_t io_control_addr;
    };
    
    InoutImpl(const ConstructorType& args);
    static int probe(TimingReceiver* tr, TimingReceiver::ActionSinks& actionSinks, TimingReceiver::EventSources& eventSources);
    
    // iOutputActionSink
    void WriteOutput(bool value);
    bool ReadOutput();
    bool getOutputEnable() const;
    void setOutputEnable(bool val);
    bool getOutputEnableAvailable() const;
    bool getSpecialPurposeOut() const;
    void setSpecialPurposeOut(bool val);
    bool getSpecialPurposeOutAvailable() const;
    bool getBuTiSMultiplexer() const;
    void setBuTiSMultiplexer(bool val);
    Glib::ustring getLogicLevelOut() const;
    Glib::ustring getTypeOut() const;
    
    // iInputEventSource
    bool ReadInput(); // done
    bool getInputTermination() const;
    void setInputTermination(bool val);
    bool getInputTerminationAvailable() const;
    bool getSpecialPurposeIn() const;
    void setSpecialPurposeIn(bool val);
    bool getSpecialPurposeInAvailable() const;
    Glib::ustring getLogicLevelIn() const;
    Glib::ustring getTypeIn() const;
    
    // iInputEventSource
    guint64 getResolution() const;
    
    sigc::signal< void, bool > OutputEnable;
    sigc::signal< void, bool > SpecialPurposeOut;
    sigc::signal< void, bool > BuTiSMultiplexer;
    sigc::signal< void, bool > InputTermination;
    sigc::signal< void, bool > SpecialPurposeIn;
    
  protected:
    TimingReceiver* tr;
    unsigned io_channel;
    unsigned io_index;
    unsigned io_special_purpose;
    unsigned io_logic_level;
    bool io_oe_available;
    bool io_term_available;
    bool io_spec_out_available;
    bool io_spec_in_available;
    eb_address_t io_control_addr; 
    Glib::ustring getLogicLevel() const;
    Glib::ustring getType() const;
};

}

#endif
