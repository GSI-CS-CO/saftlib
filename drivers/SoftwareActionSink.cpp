#define ETHERBONE_THROWS 1

#include <cassert>
#include "RegisteredObject.h"
#include "SoftwareActionSink.h"
#include "SoftwareCondition.h"
#include "TimingReceiver.h"
#include "eca_queue_regs.h"
#include "eca_flags.h"
#include "src/clog.h"

namespace saftlib {

SoftwareActionSink::SoftwareActionSink(const ConstructorType& args)
 : ActionSink(args.objectPath, args.dev, args.name, args.channel, args.num, args.destroy), queue(args.queue)
{
}

Glib::RefPtr<SoftwareActionSink> SoftwareActionSink::create(const ConstructorType& args)
{
  return RegisteredObject<SoftwareActionSink>::create(args.objectPath, args);
}

const char *SoftwareActionSink::getInterfaceName() const
{
  return "SoftwareActionSink";
}

void SoftwareActionSink::receiveMSI(guint8 code)
{
  // Intercept valid action counter increase
  if (code == ECA_VALID) {
    updateAction(0); // increase the counter, rearming the MSI
    
    eb_data_t flags, rawNum, event_hi, event_lo, param_hi, param_lo, 
              tag, tef, deadline_hi, deadline_lo, executed_hi, executed_lo;
    
    etherbone::Cycle cycle;
    cycle.open(dev->getDevice());
    cycle.read(queue + ECA_QUEUE_FLAGS_GET,       EB_DATA32, &flags);
    cycle.read(queue + ECA_QUEUE_NUM_GET,         EB_DATA32, &rawNum);
    cycle.read(queue + ECA_QUEUE_EVENT_ID_HI_GET, EB_DATA32, &event_hi);
    cycle.read(queue + ECA_QUEUE_EVENT_ID_LO_GET, EB_DATA32, &event_lo);
    cycle.read(queue + ECA_QUEUE_PARAM_HI_GET,    EB_DATA32, &param_hi);
    cycle.read(queue + ECA_QUEUE_PARAM_LO_GET,    EB_DATA32, &param_lo);
    cycle.read(queue + ECA_QUEUE_TAG_GET,         EB_DATA32, &tag);
    cycle.read(queue + ECA_QUEUE_TEF_GET,         EB_DATA32, &tef);
    cycle.read(queue + ECA_QUEUE_DEADLINE_HI_GET, EB_DATA32, &deadline_hi);
    cycle.read(queue + ECA_QUEUE_DEADLINE_LO_GET, EB_DATA32, &deadline_lo);
    cycle.read(queue + ECA_QUEUE_EXECUTED_HI_GET, EB_DATA32, &executed_hi);
    cycle.read(queue + ECA_QUEUE_EXECUTED_LO_GET, EB_DATA32, &executed_lo);
    cycle.write(queue + ECA_QUEUE_POP_OWR, EB_DATA32, 1);
    cycle.close();
    
    guint64 id       = guint64(event_hi)    << 32 | event_lo;
    guint64 param    = guint64(param_hi)    << 32 | param_lo;
    guint64 deadline = guint64(deadline_hi) << 32 | deadline_lo;
    guint64 executed = guint64(executed_hi) << 32 | executed_lo;
    
    if ((flags & (1<<ECA_VALID)) == 0) {
      clog << kLogErr << "SoftwareActionSink: MSI for increase in VALID_COUNT did not correspond to a valid action in the queue" << std::endl;
      return;
    }
    
    if (rawNum != num) {
      clog << kLogErr << "SoftwareActionSink: MSI dispatched to wrong queue" << std::endl;
      return;
    }
    
    // Emit the Action
    Conditions::iterator it = conditions.find(tag);
    if (it == conditions.end()) {
      // This can happen if the user deletes a condition at the same time a match arrives
      // => Just silently discard the action on this race condition
      return;
    } 
    
    Glib::RefPtr<SoftwareCondition> softwareCondition =
      Glib::RefPtr<SoftwareCondition>::cast_dynamic(it->second);
    if (!softwareCondition) {
      clog << kLogErr << "SoftwareActionSink: a Condition was not a SoftwareCondition" << std::endl;
      return;
    }
    
    // Inform clients
    softwareCondition->Action(id, param, deadline, executed, flags & 0xF);
    
  } else {
    // deal with the MSI the normal way
    ActionSink::receiveMSI(code);
  }
}

Glib::ustring SoftwareActionSink::NewCondition(bool active, guint64 id, guint64 mask, gint64 offset)
{
  return NewConditionHelper(active, id, mask, offset, 0, true,
    sigc::ptr_fun(&SoftwareCondition::create));
}

}
