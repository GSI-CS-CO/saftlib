/*  Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#include "SoftwareCondition.hpp"

namespace saftlib {

SoftwareCondition::SoftwareCondition(ActionSink *sink, unsigned number, bool active, uint64_t id, uint64_t mask, int64_t offset, saftbus::Container *container = nullptr)
 : Condition(sink, number, active, id, mask, offset, number, container)
{
  // std::cerr << "SoftwareCondition::SoftwareCondition()" << std::endl;
}

}
