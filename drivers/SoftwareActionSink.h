#ifndef SOFTWARE_ACTION_SINK_H
#define SOFTWARE_ACTION_SINK_H

#include "interfaces/SoftwareActionSink.h"
#include "ActionSink.h"

namespace saftlib {

class SoftwareActionSink : public ActionSink, public iSoftwareActionSink
{
  public:
    typedef SoftwareActionSink_Service ServiceType;
    struct ConstructorType {
      TimingReceiver* dev;
      int channel;
      sigc::slot<void> destroy;
    };
    
    static Glib::RefPtr<SoftwareActionSink> create(const Glib::ustring& objectPath, ConstructorType args);
    
    const char *getInterfaceName() const;
    
    // iActionSink overrides
    guint32 getConflictCount() const;
    guint32 getLateCount() const;
    guint32 getDelayedCount() const;
    guint32 getActionCount() const;
    bool getExecuteLateActions() const;
    bool getGenerateDelayed() const;
    void setConflictCount(guint32 val);
    void setLateCount(guint32 val);
    void setDelayedCount(guint32 val);
    void setActionCount(guint32 val);
    void setExecuteLateActions(bool val);
    void setGenerateDelayed(bool val);
    
    // iSoftwareAcitonSink
    Glib::ustring NewCondition(bool active, guint64 id, guint64 mask, gint64 offset, guint32 guards);
    
    void emit(guint64 id, guint64 param, guint64 time, guint64 overtime, guint64 offset, bool late, bool delayed);
    
  protected:
    SoftwareActionSink(ConstructorType args);
    
    guint32 conflictCount, lateCount, delayedCount, actionCount;
    bool executeLate, generateDelayed;
    
    guint64 lastID;
    guint64 lastParam;
    guint64 lastTime;
};

}

#endif
