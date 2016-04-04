#ifndef TIMING_RECEIVER_H
#define TIMING_RECEIVER_H

#include "OpenDevice.h"
#include "ActionSink.h"
#include "FunctionGenerator.h"
#include "interfaces/TimingReceiver.h"

namespace saftlib {

class TimingReceiver : public iTimingReceiver, public iDevice, public Glib::Object {
  public:
    struct ConstructorType {
      Device device;
      Glib::ustring name;
      Glib::ustring etherbonePath;
      eb_address_t base;
      eb_address_t stream;
      eb_address_t pps;
    };
    typedef TimingReceiver_Service ServiceType;
    
    static void probe(OpenDevice& od);
    TimingReceiver(ConstructorType args);
    ~TimingReceiver();
    
    // iDevice
    void Remove();
    Glib::ustring getEtherbonePath() const;
    Glib::ustring getName() const;
    
    // iTimingReceiver
    Glib::ustring NewSoftwareActionSink(const Glib::ustring& name);
    void InjectEvent(guint64 event, guint64 param, guint64 time);
    guint64 ReadCurrentTime();
    bool getLocked() const;
    std::map< Glib::ustring, Glib::ustring > getSoftwareActionSinks() const;
    std::map< Glib::ustring, Glib::ustring > getOutputs() const;
    std::map< Glib::ustring, Glib::ustring > getInputs() const;
    std::map< Glib::ustring, Glib::ustring > getInoutputs() const;
    std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > getInterfaces() const;
    guint32 getFree() const;
    
    // Compile the condition table
    void compile();

    // provided by RegisteredObject
    virtual const Glib::ustring& getSender() const = 0;
    virtual const Glib::ustring& getObjectPath() const = 0;
    virtual const Glib::RefPtr<Gio::DBus::Connection>& getConnection() const = 0;
    
    // Allow hardware access to the underlying device
    Device& getDevice() { return device; }
    eb_address_t getBase() { return base; }
    guint64 ReadRawCurrentTime();
    
    // public type, even though the member is private
    typedef std::pair<unsigned, unsigned> SinkKey; // (channel, num)
    typedef std::map< SinkKey, Glib::RefPtr<ActionSink> >  ActionSinks;
    
  protected:
    mutable saftlib::Device device;
    Glib::ustring name;
    Glib::ustring etherbonePath;
    eb_address_t base;
    eb_address_t stream;
    eb_address_t pps;
    guint64 sas_count;
    eb_address_t arrival_irq;
    eb_address_t generator_irq;
    mutable bool locked;
    sigc::connection pollConnection;
    unsigned channels;
    unsigned search_size;
    unsigned walker_size;
    unsigned max_conditions;
    unsigned used_conditions;
    std::vector<eb_address_t> channel_msis;
    std::vector<eb_address_t> queue_addresses;
    
    typedef std::map< Glib::ustring, Glib::RefPtr<Owned> > Owneds;
    typedef std::map< Glib::ustring, Owneds >              OtherStuff;
    
    ActionSinks actionSinks;
    OtherStuff otherStuff;
    
    // Polling method
    bool poll();
    
    void do_remove(SinkKey key);
    void setHandler(unsigned channel, bool enable, eb_address_t address);
    void msiHandler(unsigned channel, eb_data_t);
  
  friend class ActionSink;
};

}

#endif
