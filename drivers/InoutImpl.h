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
      unsigned io_delay;
      unsigned io_logic_level;
      bool io_oe_available;
      bool io_term_available;
      bool io_spec_available;
      eb_address_t io_control_addr;
    };
    
    InoutImpl(ConstructorType args);
    static int probe(TimingReceiver* tr, std::map< Glib::ustring, Glib::RefPtr<ActionSink> >& actionSinks);
    
    // iOutputActionSink
    Glib::ustring NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards, bool on);
    void WriteOutput(bool value); // done
    bool ReadOutput(); // done
    bool getOutputEnable() const; // done
    void setOutputEnable(bool val); // done 
    bool getOutputEnableAvailable() const; // done
    bool getSpecialPurposeOutAvailable() const;
    Glib::ustring getLogicLevelOut() const; // done
    
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
    bool getInputTermination() const; // done
    void setStableTime(guint32 val);
    void setInputTermination(bool val); // done
    bool getInputTerminationAvailable() const;
    bool getSpecialPurposeInAvailable() const;
    Glib::ustring getLogicLevelIn() const;
    
  protected:
    unsigned io_channel;
    unsigned io_index;
    unsigned io_delay;
    unsigned io_logic_level;
    bool io_oe_available;
    bool io_term_available;
    bool io_spec_available;
    eb_address_t io_control_addr; 
    Glib::ustring getLogicLevel() const;
};

}

#endif
