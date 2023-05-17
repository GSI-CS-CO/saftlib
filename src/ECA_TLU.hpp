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

#ifndef saftlib_ECA_TLU_HPP_
#define saftlib_ECA_TLU_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <memory>
#include <map>

#include <saftbus/service.hpp>

#include "SdbDevice.hpp"

namespace saftlib {

class Input;

/// @brief Interface to the ECA_TLU SdbInterface
class ECA_TLU : public SdbDevice {
	saftbus::Container *container;
	std::vector<std::unique_ptr<Input> > inputs;

public:
	ECA_TLU(etherbone::Device &device, saftbus::Container *container = nullptr);
	~ECA_TLU();

	/// @brief add source and let ECA take ownership of the sink object
	void addInput(std::unique_ptr<Input> input);

	// @saftbus-export
	std::map< std::string, std::string > getInputs() const;

	void configInput(unsigned channel,
	                 bool enable,
	                 uint64_t event,
	                 uint32_t stable);
};

}

#endif
