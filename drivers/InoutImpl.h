#ifndef INOUTPUT_IMPL_H
#define INOUTPUT_IMPL_H

#include <etherbone.h>
#include "interfaces/iOutputActionSink.h"
#include "interfaces/iEventSource.h"
#include "interfaces/iInputEventSource.h"
#include "ActionSink.h"

namespace saftlib {

class InoutImpl : public ActionSink, public iOutputActionSink, public iEventSource, public iInputEventSource
{
  public:
    struct ConstructorType {
      TimingReceiver* dev;
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
    
    InoutImpl(ConstructorType args);
    static int probe(TimingReceiver* tr, std::map< Glib::ustring, Glib::RefPtr<ActionSink> >& actionSinks);
    
    // iOutputActionSink
    Glib::ustring NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards, bool on);
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
    
    // iEventSource
    guint64 getResolution() const;
    guint32 getEventBits() const;
    bool getEventEnable() const;
    guint64 getEventPrefix() const;
    void setEventEnable(bool val);
    void setEventPrefix(guint64 val);
    
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
    
  protected:
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
