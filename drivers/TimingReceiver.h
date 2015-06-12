#ifndef TIMING_RECEIVER_H
#define TIMING_RECEIVER_H

#include "OpenDevice.h"
#include "SoftwareActionSink.h"
#include "interfaces/TimingReceiver.h"

namespace saftlib {

class TimingReceiver : public iTimingReceiver, public iDevice, public Glib::Object {
  public:
    typedef OpenDevice& ConstructorType;
    typedef TimingReceiver_Service ServiceType;
    
    static void probe(OpenDevice& od);
    TimingReceiver(OpenDevice& device);
    
    // iDevice
    void Remove();
    Glib::ustring getEtherbonePath() const;
    Glib::ustring getName() const;
    
    // iTimingReceiver
    Glib::ustring NewSoftwareActionSink(const Glib::ustring& name);
    void InjectEvent(guint64 event, guint64 param, guint64 time);
    guint64 getCurrentTime() const;
    std::map< Glib::ustring, Glib::ustring > getSoftwareActionSinks() const;
    std::map< Glib::ustring, Glib::ustring > getOutputs() const;
    std::map< Glib::ustring, Glib::ustring > getInputs() const;
    std::map< Glib::ustring, Glib::ustring > getInoutputs() const;
    std::vector< Glib::ustring > getGuards() const;
    std::map< Glib::ustring, std::vector< Glib::ustring > > getInterfaces() const;
    guint32 getFree() const;
    
    // Compile the condition table
    void compile();

    // provided by RegisteredObject
    virtual const Glib::ustring& getSender() const = 0;
    virtual const Glib::ustring& getObjectPath() const = 0;
    virtual const Glib::RefPtr<Gio::DBus::Connection>& getConnection() const = 0;
    
  protected:
    saftlib::Device dev;
    Glib::ustring name;
    Glib::ustring etherbonePath;
    
    std::map< Glib::ustring, Glib::RefPtr<SoftwareActionSink> > softwareActionSinks;
    
    void do_remove(Glib::ustring name);
};

}

#endif
