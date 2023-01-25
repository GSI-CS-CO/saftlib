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
/// @brief provide additional functionality for a TimingReciever 
/// 
/// By inheriting from this class, and loading an instance of it into a TimingReceiver
/// object (using the TimingReceiver::installAddon method) it is possible to extend the 
/// functionality of a TimingReceiver (while using its ressources) in a very general way. 
/// This may be a firmware driver, interacting with one ore many LM32 cores, but 
/// can also be something completetly different. 
/// For example, take control over several IOs and implement a bit-bang serial protocol. 
/// The addon mechanism imposes no artificial limits. 
/// It passes a pointer to a TimingReceiver object to the TimingReceiverAddon constructor. 
/// The only thing any Addon needs to do is provide a list of Interfaces and the 
/// ObjectPaths such that a remote user can instanciate the services over saftubs proxies.
class TimingReceiverAddon {
public:
    virtual ~TimingReceiverAddon();

    virtual std::map<std::string, std::string> getObjects() = 0;

};

}

#endif
