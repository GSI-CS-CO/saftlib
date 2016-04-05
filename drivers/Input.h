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
    Glib::ustring getOutput() const;
    // Property setters
    void setStableTime(guint32 val);
    void setInputTermination(bool val);
    void setSpecialPurposeIn(bool val);
    
    // Property signals
    //  sigc::signal< void, guint32 > StableTime;
    //  sigc::signal< void, bool > InputTermination;
    //  sigc::signal< void, bool > SpecialPurposeIn;
    
  protected:
    Input(const ConstructorType& args);
    Glib::RefPtr<InoutImpl> impl;
    Glib::ustring partnerPath;
};

}

#endif /* INPUT_H */
