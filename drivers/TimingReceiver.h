#ifndef TIMING_RECEIVER_H
#define TIMING_RECEIVER_H

#include "OpenDevice.h"
#include "ActionSink.h"
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
      eb_address_t queue;
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
    guint64 getCurrentTime() const;
    bool getLocked() const;
    std::map< Glib::ustring, Glib::ustring > getSoftwareActionSinks() const;
    std::map< Glib::ustring, Glib::ustring > getOutputs() const;
    std::map< Glib::ustring, Glib::ustring > getInputs() const;
    std::map< Glib::ustring, Glib::ustring > getInoutputs() const;
    std::vector< Glib::ustring > getGuards() const;
    std::map< Glib::ustring, std::map< Glib::ustring, Glib::ustring > > getInterfaces() const;
    guint32 getFree() const;
    
    // Compile the condition table
    void compile();
    int getQueueSize() { return queue_size; }
    int getTableSize() { return table_size; }

    // provided by RegisteredObject
    virtual const Glib::ustring& getSender() const = 0;
    virtual const Glib::ustring& getObjectPath() const = 0;
    virtual const Glib::RefPtr<Gio::DBus::Connection>& getConnection() const = 0;
    
    // Polling method
    bool poll();
    
    // Allow hardware access to the underlying device
    Device& getDevice() { return device; }
    
  protected:
    mutable saftlib::Device device;
    Glib::ustring name;
    Glib::ustring etherbonePath;
    eb_address_t base;
    eb_address_t stream;
    eb_address_t queue;
    eb_address_t pps;
    int sas_count;
    eb_address_t overflow_irq;
    eb_address_t arrival_irq;
    mutable bool locked;
    sigc::connection pollConnection;
    int channels;
    int queue_size;
    int table_size;
    int aq_channel;
    std::map< Glib::ustring, Glib::RefPtr<ActionSink> > actionSinks;
    std::vector<guint64> tag2delay;
    
    void do_remove(Glib::ustring name);
    void setHandlers(bool enable, eb_address_t arrival = 0, eb_address_t overflow = 0);
    void overflow_handler(eb_data_t);
    void arrival_handler(eb_data_t); 
  
  friend class ActionSink;
};

}

#endif
