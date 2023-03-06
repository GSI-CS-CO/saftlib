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

#include "EmbeddedCPUCondition.hpp"

namespace saftlib {

EmbeddedCPUCondition::EmbeddedCPUCondition(ActionSink *sink, unsigned number, bool active, uint64_t id, uint64_t mask, int64_t offset, uint32_t tag, saftbus::Container *container)
 : Owned(container), Condition(sink, number, active, id, mask, offset, tag)
{
  std::cerr << "EmbeddedCPUCondition::EmbeddedCPUCondition()" << std::endl;
}

uint32_t EmbeddedCPUCondition::getTag() const
{
  return tag;
}

void EmbeddedCPUCondition::setTag(uint32_t val)
{
  ownerOnly();
  if (val == tag) return;
  uint32_t old = tag;
  
  tag = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    tag = old;
    throw;
  }
}

}
