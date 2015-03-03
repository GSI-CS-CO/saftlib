#define ETHERBONE_THROWS 1

#include "ObjectRegistry.h"
#include "Driver.h"
#include "ECA.h"
#include <iostream>
#include <sstream>
#include <eca.h>

namespace saftlib {

class ECA : public RegisteredObject<ECA_Service>
{
  public:
    ECA(Devices& devices);
    void Poke();
    
    void ready(eb_data_t);
    
  protected:
    eb_address_t irq;
    std::vector<GSI_ECA::ECA> ecas;
};

ECA::ECA(Devices& devices)
 : RegisteredObject<ECA_Service>("/de/gsi/saftlib/ECA")
{
  // completely and totally unsafe!
  GSI_ECA::ECA::probe(devices[0], ecas);
  
  setFrequency(ecas[0].frequency());
  setName(ecas[0].name);
  
  ecas[0].disable(false);
  ecas[0].channels[1].drain(false);
  ecas[0].channels[1].freeze(false);
  
  GSI_ECA::ActionQueue& aq = ecas[0].channels[1].queue.front();
  irq = devices[0].request_irq(sigc::mem_fun(*this, &ECA::ready));
  aq.hook_arrival(true, irq);
  
  GSI_ECA::ActionEntry ae;
  aq.refresh();
  while (aq.queued_actions) aq.pop(ae);
}

void ECA::Poke()
{
  setName("xx");
}

void ECA::ready(eb_data_t)
{
  GSI_ECA::ActionEntry ae;
  ecas[0].channels[1].queue.front().refresh();
  ecas[0].channels[1].queue.front().pop(ae);
  std::ostringstream stream;
  stream << "ECA says: " << std::hex << ae.event;
}

static Driver<ECA> eca;

} // saftlib
