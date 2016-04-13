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
#ifndef SAFTLIB_DRIVER_H
#define SAFTLIB_DRIVER_H

#include "OpenDevice.h"

namespace saftlib {

class DriverBase
{
  public:
    DriverBase() { }
    void insert_self();
    void remove_self();
    
  protected:
    virtual void probe(OpenDevice& od) = 0;
    
  private:
    DriverBase *next;
    
    // non-copyable
    DriverBase(const DriverBase&);
    DriverBase& operator = (const DriverBase&);
  
  friend class Drivers;
};

template <typename T>
class Driver : private DriverBase
{
  public:
    Driver() { insert_self(); }
    ~Driver() { remove_self(); }
  
  private:
    void probe(OpenDevice& od) { return T::probe(od); }
};

class Drivers
{
  public:
    static void probe(OpenDevice& od);
};

}

#endif
