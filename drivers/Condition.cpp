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

#include "Condition.h"
#include "ActionSink.h"
#include "TimingReceiver.h"

namespace saftlib {

Condition::Condition(const Condition_ConstructorType& args)
 : Owned(args.objectPath, args.destroy), sink(args.sink), 
   id(args.id), mask(args.mask), offset(args.offset), tag(args.tag),
   acceptLate(false), acceptEarly(false), acceptConflict(false), acceptDelayed(true),
   active(args.active)
{
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
  ownerOnly();
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
  ownerOnly();
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
  ownerOnly();
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
  ownerOnly();
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
  ownerOnly();
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
  ownerOnly();
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
  ownerOnly();
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
  ownerOnly();
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
