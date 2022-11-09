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

#include "ECA_TLU.hpp"
#include "Input.hpp"


#include <saftbus/error.hpp>

#include <iostream>

#include "eca_tlu_regs.h"

namespace saftlib {

ECA_TLU::ECA_TLU(etherbone::Device &dev, saftbus::Container *cont) 
	: device(dev)
	, container(cont)
{
	std::vector<sdb_device> eca_tlu_devs;
	device.sdb_find_by_identity(ECA_TLU_SDB_VENDOR_ID, ECA_TLU_SDB_DEVICE_ID, eca_tlu_devs);
	if (eca_tlu_devs.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no eca_tlu device found on hardware");
	}
	if (eca_tlu_devs.size() > 1) {
		std::cerr << "more than one eca_tlu device found on hardware, taking the first one" << std::endl;
	}
	eca_tlu = static_cast<eb_address_t>(eca_tlu_devs[0].sdb_component.addr_first);
}

ECA_TLU::~ECA_TLU() {
	// std::cerr << "ECA_TLU::~ECA_TLU() " << std::endl;
	if (container) {
		for (auto &input: inputs) {
			if (input) {
				// std::cerr << "   remove " << input->getObjectPath() << std::endl;
				try {
					container->remove_object(input->getObjectPath());
				} catch (saftbus::Error &e) {
					std::cerr << "removal attempt failed: " << e.what() << std::endl;
				}
			}
		}
	}
}


void ECA_TLU::addInput(std::unique_ptr<Input> input) {
	// std::cerr << "ECA_TLU::addInput() " << input->getObjectPath() << std::endl;
	inputs.push_back(std::move(input));
}


void ECA_TLU::configInput(unsigned channel,
	                      bool enable,
	                      uint64_t event,
	                      uint32_t stable)
{
	// std::cout << "ECA_TLU::configInput(channel=" << channel <<", enable=" << enable << ", event=" << event << ", stable=" << stable << ")" << std::endl;
	etherbone::Cycle cycle;
	cycle.open(device);
	cycle.write(eca_tlu + ECA_TLU_INPUT_SELECT_RW, EB_DATA32, channel);
	cycle.write(eca_tlu + ECA_TLU_ENABLE_RW,       EB_DATA32, enable?1:0);
	cycle.write(eca_tlu + ECA_TLU_STABLE_RW,       EB_DATA32, stable/8 - 1);
	cycle.write(eca_tlu + ECA_TLU_EVENT_HI_RW,     EB_DATA32, event >> 32);
	cycle.write(eca_tlu + ECA_TLU_EVENT_LO_RW,     EB_DATA32, (uint32_t)event);
	cycle.write(eca_tlu + ECA_TLU_WRITE_OWR,       EB_DATA32, 1);
	cycle.close();
}

std::map< std::string, std::string > ECA_TLU::getInputs() const
{
	std::map< std::string, std::string > out;
	for (auto &input: inputs) {
		out[input->getObjectName()] = input->getObjectPath();
	}
	return out;
}


} // namespace