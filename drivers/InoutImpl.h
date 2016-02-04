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
    
    // iEventSource
    guint64 getResolution() const ;
    guint32 getEventBits() const;
    bool getEventEnable() const;
    guint64 getEventPrefix() const;
    void setEventEnable(bool val);
    void setEventPrefix(guint64 val);
    
    // iInputEventSource
    bool ReadInput();
    guint32 getStableTime() const;
    bool getTermination() const;
    void setStableTime(guint32 val);
    void setTermination(bool val);
      
  protected:
    unsigned io_channel; /* Channel GPIO or LVDS */
    unsigned io_index; /* Channel index (GPIO or LVDS) */
    bool io_oe_available; /* Does this IO have an output enable option? */
    bool io_term_available; /* Does this IO have an termination option? */
    bool io_spec_available; /* Does this IO have an special option? */
    eb_address_t io_control_addr; /* Wishbone address of the io_control module */
};

}

#endif
