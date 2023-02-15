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

#include "WbmActionSink.hpp"
#include "ECA.hpp"
#include "eca_ac_wbm_regs.h"

#include "Condition.hpp"
#include "WbmCondition.hpp"
#include "WbmCondition_Service.hpp"

#include <saftbus/error.hpp>

#include <cassert>
#include <sstream>
#include <memory>

namespace saftlib {

WbmActionSink::WbmActionSink(etherbone::Device &dev
	                              ,ECA &eca
                                  , const std::string &obj_path
                                  , const std::string &name
                                  , unsigned channel, eb_address_t acwbm_address
                                  , saftbus::Container *container)
	: ActionSink(eca, obj_path, name, channel, 0, container), device(dev), acwbm(acwbm_address)
{
}



std::string WbmActionSink::NewCondition(bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag)
{
	return NewConditionHelper<WbmCondition>(active, id, mask, offset, tag, container);
}


void WbmActionSink::ExecuteMacro(uint32_t idx) {
  eb_data_t data = idx;
  device.write(acwbm + SLAVE_EXEC_OWR, EB_DATA32, data);
}
void WbmActionSink::RecordMacro(uint32_t idx, const std::vector< std::vector< uint32_t > >& commands) {
  etherbone::Cycle cycle;
  cycle.open(device);
  for(unsigned cmd_idx = 0; cmd_idx < commands.size(); ++cmd_idx) {
    if (commands[cmd_idx].size() != 3) {
      throw saftbus::Error(saftbus::Error::INVALID_ARGS, "each command must consist of exactly three words");
    }
    eb_data_t data = idx;
    cycle.write(acwbm + SLAVE_REC_OWR, EB_DATA32, data); // start recording
    for (unsigned elm_idx = 0; elm_idx < 3; ++elm_idx) {
      data = commands[cmd_idx][elm_idx];
      cycle.write(acwbm + SLAVE_REC_FIFO_OWR, EB_DATA32, data);
    }
    cycle.write(acwbm + SLAVE_REC_OWR, EB_DATA32, data); // stop recording
  }
  cycle.close();
}
void WbmActionSink::ClearMacro(uint32_t idx) {
  eb_data_t data = idx;
  device.write(acwbm + SLAVE_CLEAR_IDX_OWR, EB_DATA32, data);
}
void WbmActionSink::ClearAllMacros() {
  eb_data_t data = 1;
  device.write(acwbm + SLAVE_CLEAR_ALL_OWR, EB_DATA32, data);
}
unsigned char WbmActionSink::getStatus() const {
  eb_data_t data;
  device.read(acwbm + SLAVE_STATUS_GET, EB_DATA32, &data);
  return data;
}
uint32_t WbmActionSink::getMaxMacros() const {
  eb_data_t data;
  device.read(acwbm + SLAVE_MAX_MACROS_GET, EB_DATA32, &data);
  return data;
}
uint32_t WbmActionSink::getMaxSpace() const {
  eb_data_t data;
  device.read(acwbm + SLAVE_MAX_SPACE_GET, EB_DATA32, &data);
  return data;
}
bool WbmActionSink::getEnable() const {
  eb_data_t data;
  device.read(acwbm + SLAVE_ENABLE_GET, EB_DATA32, &data);
  return data;
}
uint32_t WbmActionSink::getLastExecutedIdx() const {
  eb_data_t data;
  device.read(acwbm + SLAVE_LAST_EXEC_GET, EB_DATA32, &data);
  return data;
}
uint32_t WbmActionSink::getLastRecordedIdx() const {
  eb_data_t data;
  device.read(acwbm + SLAVE_LAST_REC_GET, EB_DATA32, &data);
  return data;
}
void WbmActionSink::setEnable(bool val) {
  eb_data_t data = val;
  if (val) {
    device.write(acwbm + SLAVE_ENABLE_SET, EB_DATA32, data);
  } else {
    device.write(acwbm + SLAVE_ENABLE_CLR, EB_DATA32, data);
  }
}




}
