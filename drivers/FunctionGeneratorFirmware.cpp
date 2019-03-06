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

#include <iostream>

#include <assert.h>

#include "RegisteredObject.h"
#include "FunctionGeneratorFirmware.h"
#include "TimingReceiver.h"
#include "fg_regs.h"
#include "clog.h"

namespace saftlib {

FunctionGeneratorFirmware::FunctionGeneratorFirmware(const ConstructorType& args)
 : Owned(args.objectPath),
   receiver(args.receiver)
{
}

FunctionGeneratorFirmware::~FunctionGeneratorFirmware()
{
}

// pass sigc signals from impl class to dbus
// to reduce traffic only generate signals if we have an owner
std::map<std::string, std::string> FunctionGeneratorFirmware::Scan()
{
  return std::map<std::string, std::string>();
}



}
