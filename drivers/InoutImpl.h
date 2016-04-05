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
    
    // iInputEventSource
    bool ReadInput(); // done
    guint32 getStableTime() const;
    bool getInputTermination() const;
    void setStableTime(guint32 val);
    void setInputTermination(bool val);
    bool getInputTerminationAvailable() const;
    bool getSpecialPurposeIn() const;
    void setSpecialPurposeIn(bool val);
    bool getSpecialPurposeInAvailable() const;
    Glib::ustring getLogicLevelIn() const;
    
    sigc::signal< void, bool > OutputEnable;
    sigc::signal< void, bool > SpecialPurposeOut;
    sigc::signal< void, bool > BuTiSMultiplexer;
    sigc::signal< void, guint32 > StableTime;
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
};

}

#endif
