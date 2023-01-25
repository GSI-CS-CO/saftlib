/** Copyright (C) 2022-2023 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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

#ifndef SAFTLIB_TIMING_RECEIVER_ADDON_HPP_
#define SAFTLIB_TIMING_RECEIVER_ADDON_HPP_

#include <string>
#include <map>

namespace saftlib {

class Reset;

class TimingReceiverAddon {
public:
    virtual ~TimingReceiverAddon();

    virtual std::map<std::string, std::string> getObjects() = 0;

};

}

#endif
