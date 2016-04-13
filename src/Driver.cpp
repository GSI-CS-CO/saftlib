/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#include "Driver.h"

namespace saftlib {

static DriverBase *top = 0;

void DriverBase::insert_self()
{
  next = top;
  top = this;
}

void DriverBase::remove_self()
{
  DriverBase **i;
  for (i = &top; *i != this; i = &(*i)->next) { }
  *i = next;
}

void Drivers::probe(OpenDevice& od)
{
  for (DriverBase *i = top; i; i = i->next)
    i->probe(od);
}

}
