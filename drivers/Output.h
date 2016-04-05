#ifndef OUTPUT_H
#define OUTPUT_H

#include "InoutImpl.h"
#include "ActionSink.h"
#include "interfaces/Output.h"
#include "interfaces/iOutputActionSink.h"

namespace saftlib {

class Output : public ActionSink, public iOutputActionSink
{
  public:
    typedef Output_Service ServiceType;
    struct ConstructorType {
      Glib::ustring name;
      Glib::ustring objectPath;
      Glib::ustring partnerPath;
      TimingReceiver* dev;
      unsigned channel;
      unsigned num;
      Glib::RefPtr<InoutImpl> impl;
      sigc::slot<void> destroy;
    };

    static Glib::RefPtr<Output> create(const ConstructorType& args);
    
    const char *getInterfaceName() const;
    
    // Methods
    Glib::ustring NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, bool on);
    void WriteOutput(bool value);
    bool ReadOutput();
    
    // Property getters
    bool getOutputEnable() const;
    bool getSpecialPurposeOut() const;
    bool getBuTiSMultiplexer() const;
    bool getOutputEnableAvailable() const;
    bool getSpecialPurposeOutAvailable() const;
    Glib::ustring getLogicLevelOut() const;
    Glib::ustring getInput() const;
    
    // Property setters
    void setOutputEnable(bool val);
    void setSpecialPurposeOut(bool val);
    void setBuTiSMultiplexer(bool val);
    
    // Property signals
    //   sigc::signal< void, bool > OutputEnable;
    //   sigc::signal< void, bool > SpecialPurposeOut;
    //   sigc::signal< void, bool > BuTiSMultiplexer;
  
  protected:
    Output(const ConstructorType& args);
    Glib::RefPtr<InoutImpl> impl;
    Glib::ustring partnerPath;
};

}

#endif /* OUTPUT_H */
