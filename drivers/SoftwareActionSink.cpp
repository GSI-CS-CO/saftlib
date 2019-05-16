/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

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

std::shared_ptr<SoftwareActionSink> SoftwareActionSink::create(const ConstructorType& args)
{
  return RegisteredObject<SoftwareActionSink>::create(args.objectPath, args);
}

const char *SoftwareActionSink::getInterfaceName() const
{
  return "SoftwareActionSink";
}

void SoftwareActionSink::receiveMSI(uint8_t code)
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
    
    uint64_t id       = uint64_t(event_hi)    << 32 | event_lo;
    uint64_t param    = uint64_t(param_hi)    << 32 | param_lo;
    uint64_t deadline = uint64_t(deadline_hi) << 32 | deadline_lo;
    uint64_t executed = uint64_t(executed_hi) << 32 | executed_lo;
    
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
    
    std::shared_ptr<SoftwareCondition> softwareCondition =
      std::dynamic_pointer_cast<SoftwareCondition>(it->second);
    // std::shared_ptr<SoftwareCondition> softwareCondition =
    //   std::shared_ptr<SoftwareCondition>::cast_dynamic(it->second);
    if (!softwareCondition) {
      clog << kLogErr << "SoftwareActionSink: a Condition was not a SoftwareCondition" << std::endl;
      return;
    }
    
    // Inform clients
    softwareCondition->Action(id, param, deadline, executed, flags & 0xF);
    softwareCondition->SigAction(id, param, saftlib::makeTimeTAI(deadline), saftlib::makeTimeTAI(executed), flags & 0xF);
    
  } else {
    // deal with the MSI the normal way
    ActionSink::receiveMSI(code);
  }
}

std::string SoftwareActionSink::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset)
{
  return NewConditionHelper(active, id, mask, offset, 0, true,
    sigc::ptr_fun(&SoftwareCondition::create));
}

}
