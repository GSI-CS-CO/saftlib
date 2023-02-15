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

#ifndef saftlib_ECA_EVENT_DRIVER_HPP_
#define saftlib_ECA_EVENT_DRIVER_HPP_


#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <saftbus/service.hpp>

#include "SdbDevice.hpp"

#include <memory>

namespace saftlib {


/// @brief ECA_Event (Event Injection interface of ECA) 
/// ECA_Event provides a method to inject events into ECA.
class ECA_Event : public SdbDevice {

public:
	ECA_Event(etherbone::Device &device, saftbus::Container *container = nullptr);

	/// @brief        Simulate the receipt of a timing event
	/// @param event  The event identifier which is matched against Conditions
	/// @param param  The parameter field, whose meaning depends on the event ID.
	/// @param time   The execution time for the event, added to condition offsets.
	///
	/// Sometimes it is useful to simulate the receipt of a timing event. 
	/// This allows software to test that configured conditions lead to the
	/// desired behaviour without needing the data master to send anything.
	///
	// @saftbus-export
	void InjectEventRaw(uint64_t event, uint64_t param, uint64_t time) const;
};


} // namespace 

#endif

