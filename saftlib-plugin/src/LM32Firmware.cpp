/** Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Michael Reese <m.reese@gsi.de>
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

#include "LM32Firmware.hpp"
#include "Reset.hpp"

#include <saftbus/error.hpp>

#include <iostream>

namespace saftlib {

class LM32Firmware;

LM32Firmware::LM32Firmware(Reset &rst, int idx) 
	: reset(rst), cpu_idx(idx)
{
}

void LM32Firmware::LoadProgram(const std::string &bin_filename)
{
    
}


} // namespace