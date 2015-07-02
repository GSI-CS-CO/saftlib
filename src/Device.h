#ifndef SAFTLIB_DEVICES_H
#define SAFTLIB_DEVICES_H

#include <etherbone.h>
#include <giomm.h>

namespace saftlib {

// Saftlib devices just add IRQs
class Device : public etherbone::Device {
  public:
    Device(etherbone::Device d);
    
    eb_address_t request_irq(const sigc::slot<void,eb_data_t>& slot);
    void release_irq(eb_address_t);
    
    static void hook_it_all(etherbone::Socket s);
    static sigc::connection attach(const Glib::RefPtr<Glib::MainLoop>& loop);
    
  private:
    eb_address_t low, high;
    
    typedef std::map<eb_address_t, sigc::slot<void, eb_data_t> > irqMap;
    static irqMap irqs;
    
    struct MSI { eb_address_t address, data; };
    typedef std::deque<MSI> msiQueue;
    static msiQueue msis;

  friend class IRQ_Handler;
  friend class MSI_Source;
};

}

#endif
