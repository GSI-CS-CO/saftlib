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

guint64 Condition::getID() const
{
  return id;
}

guint64 Condition::getMask() const
{
  return mask;
}

gint64 Condition::getOffset() const
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

void Condition::setID(guint64 val)
{
  ownerOnly();
  if (val == id) return;
  guint64 old = id;
  
  id = val;
  try {
    if (active) sink->compile();
    ID(id);
  } catch (...) {
    id = old;
    throw;
  }
}

void Condition::setMask(guint64 val)
{
  ownerOnly();
  if (val == mask) return;
  guint64 old = mask;
  
  mask = val;
  try {
    if (active) sink->compile();
    Mask(mask);
  } catch (...) {
    mask = old;
    throw;
  }
}

void Condition::setOffset(gint64 val)
{
  ownerOnly();
  if (val == offset) return;
  gint64 old = offset;
  
  offset = val;
  try {
    if (active) sink->compile();
    Offset(offset);
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
    AcceptLate(val);
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
    AcceptEarly(val);
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
    AcceptConflict(val);
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
    AcceptDelayed(val);
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
    Active(active);
    sink->notify(true, true);
  } catch (...) {
    active = !val;
    throw;
  }
}

}
