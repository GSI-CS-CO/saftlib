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

#ifndef saftlib_WHITE_RABBIT_HPP_
#define saftlib_WHITE_RABBIT_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

// @saftbus-export
#include <sigc++/sigc++.h>

#include "SdbDevice.hpp"

namespace saftlib {

class WhiteRabbit : public SdbDevice {
protected:
	mutable bool locked;
public:
	WhiteRabbit(etherbone::Device &device);

	/// @brief The timing receiver is locked to the timing grandmaster.
	/// @return The timing receiver is locked to the timing grandmaster.
	///
	/// Upon power-up it takes approximately one minute until the timing
	/// receiver has a correct timestamp.
	///
	// @saftbus-export
	bool getLocked() const;
	
    // @saftbus-export
    sigc::signal<void, bool> Locked;
};

}

#endif
