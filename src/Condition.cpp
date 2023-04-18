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

#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <sstream>

#include <saftbus/error.hpp>

#include "Condition.hpp"
#include "ActionSink.hpp"
#include "TimingReceiver.hpp"

namespace saftlib {

Condition::Condition(ActionSink *sink_, unsigned number_, bool active_, uint64_t id_, uint64_t mask_, int64_t offset_, uint32_t tag_, saftbus::Container *container)
 : Owned(container), 
   objectPath(sink_->getObjectPath()), sink(sink_), number(number_),
   id(id_), mask(mask_), offset(offset_), tag(tag_),
   acceptLate(false), acceptEarly(false), acceptConflict(false), acceptDelayed(true),
   active(active_)
{
  // sanity check arguments
  if (offset < sink->getMinOffset() || offset > sink->getMaxOffset())
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "offset is out of range; adjust {min,max}Offset?");
  if ((~mask & (~mask+1)) != 0)
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "mask is not a prefix");
  if ((id & mask) != id)
    throw saftbus::Error(saftbus::Error::INVALID_ARGS, "id has bits set that are not in the mask");

  objectPath.append("/_");
  std::ostringstream number_str;
  number_str << std::dec << number;
  objectPath.append(number_str.str());
}

uint64_t Condition::getID() const
{
  return id;
}

uint64_t Condition::getMask() const
{
  return mask;
}

int64_t Condition::getOffset() const
{
  return offset;
}

bool Condition::getAcceptLate() const
{
  return acceptLate;
}

bool Condition::getAcceptEarly() const
{
  return acceptEarly;
}

bool Condition::getAcceptConflict() const
{
  return acceptConflict;
}

bool Condition::getAcceptDelayed() const
{
  return acceptDelayed;
}

bool Condition::getActive() const
{
  return active;
}

void Condition::setID(uint64_t val)
{
  // ownerOnly();
  if (val == id) return;
  uint64_t old = id;
  
  id = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    id = old;
    throw;
  }
}

void Condition::setMask(uint64_t val)
{
  // ownerOnly();
  if (val == mask) return;
  uint64_t old = mask;
  
  mask = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    mask = old;
    throw;
  }
}

void Condition::setOffset(int64_t val)
{
  // ownerOnly();
  if (val == offset) return;
  int64_t old = offset;
  
  offset = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    offset = old;
    throw;
  }
}

void Condition::setAcceptLate(bool val)
{
  // ownerOnly();
  if (val == acceptLate) return;
  
  acceptLate = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    acceptLate = !val;
    throw;
  }
}


void Condition::setAcceptEarly(bool val)
{
  // ownerOnly();
  if (val == acceptEarly) return;

  acceptEarly = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    acceptEarly = !val;
    throw;
  }
}

void Condition::setAcceptConflict(bool val)
{
  // ownerOnly();
  if (val == acceptConflict) return;

  acceptConflict = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    acceptConflict = !val;
    throw;
  }
}

void Condition::setAcceptDelayed(bool val)
{
  // ownerOnly();
  if (val == acceptDelayed) return;
  
  acceptDelayed = val;
  try {
    if (active) sink->compile();
  } catch (...) {
    acceptDelayed = !val;
    throw;
  }
}

void Condition::setActive(bool val)
{
  // ownerOnly();
  if (val == active) return;
  
  active = val;
  try {
    sink->compile();
  } catch (...) {
    active = !val;
    throw;
  }
}

}
