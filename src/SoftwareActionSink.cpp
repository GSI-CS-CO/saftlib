/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
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

#include "SoftwareActionSink.hpp"
#include "ECA.hpp"
#include "eca_queue_regs.h"
#include "eca_flags.h"

#include "Condition.hpp"
#include "SoftwareCondition.hpp"
#include "SoftwareCondition_Service.hpp"


#include <cassert>
#include <sstream>
#include <memory>

namespace saftlib {

SoftwareActionSink::SoftwareActionSink(ECA &eca
			                         , const std::string &obj_path
                                     , const std::string &name
                                     , unsigned channel, unsigned num, eb_address_t queue_address
                                     , saftbus::Container *container)
	: ActionSink(eca, obj_path, name, channel, num, container), queue(queue_address)
{
}


// SoftwareActionSink::SoftwareActionSink(const std::string &object_path, TimingReceiver *dev, const std::string &name, unsigned channel, unsigned num, eb_address_t queue, saftbus::Container *container)
//  : ActionSink(object_path, dev, name, channel, num, container), queue(queue)
// {
// }

void SoftwareActionSink::receiveMSI(uint8_t code)
{
	// std::cerr << "SoftwareActionSink::receiveMSI " << (int)code << std::endl;
	// Intercept valid action counter increase
	if (code == ECA_VALID) {
		// std::cerr << "ECA_VALID" << std::endl;
		// DRIVER_LOG("MSI-ECA_VALID",-1, code);
		updateAction(); // increase the counter, rearming the MSI
		
		eb_data_t flags, rawNum, event_hi, event_lo, param_hi, param_lo, 
							tag, tef, deadline_hi, deadline_lo, executed_hi, executed_lo;
		
		// std::cerr << "read data" << std::endl;
		etherbone::Cycle cycle;
		cycle.open(eca.get_device());
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
		// std::cerr << "read done" << std::endl;
		
		uint64_t id       = uint64_t(event_hi)    << 32 | event_lo;
		uint64_t param    = uint64_t(param_hi)    << 32 | param_lo;
		uint64_t deadline = uint64_t(deadline_hi) << 32 | deadline_lo;
		uint64_t executed = uint64_t(executed_hi) << 32 | executed_lo;
		
		if ((flags & (1<<ECA_VALID)) == 0) {
			std::cerr << "SoftwareActionSink: MSI for increase in VALID_COUNT did not correspond to a valid action in the queue" << std::endl;
			return;
		}
		
		if (rawNum != num) {
			std::cerr << "SoftwareActionSink: MSI dispatched to wrong queue" << std::endl;
			return;
		}
		
		// Emit the Action
		Conditions::iterator it = conditions.find(tag);
		if (it == conditions.end()) {
			// This can happen if the user deletes a condition at the same time a match arrives
			// => Just silently discard the action on this race condition
			return;
		} 
		
		if (!it->second) {
			std::cerr << "SoftwareActionSink: a Condition was not a SoftwareCondition" << std::endl;
			return;
		}
		
		// DRIVER_LOG("deadline",-1, deadline);
		// DRIVER_LOG("id",      -1, id);
		// Inform clients
		// softwareCondition->Action(id, param, deadline, executed, flags & 0xF);
		// std::cerr << "cast" << std::endl;
		Condition* cond = it->second.get();
		SoftwareCondition* sw_cond = dynamic_cast<SoftwareCondition*>(cond);
		// std::cerr << "SigAction" << std::endl;
		sw_cond->SigAction(id, param, saftlib::makeTimeTAI(deadline), saftlib::makeTimeTAI(executed), flags & 0xF);
		
	} else {
		// std::cerr << "not ECA_VALID" << std::endl;
		// DRIVER_LOG("MSI-ECA_NOT_VALID",-1, code);
		// deal with the MSI the normal way
		ActionSink::receiveMSI(code);
	}
}

SoftwareCondition * SoftwareActionSink::getCondition(const std::string object_path) {
	return dynamic_cast<SoftwareCondition*>(ActionSink::getCondition(object_path));
}


std::string SoftwareActionSink::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset)
{
	ownerOnly();
	return NewConditionHelper<SoftwareCondition>(active, id, mask, offset, container);
}

}
