#include "ECA_TLU.hpp"

#include <saftbus/error.hpp>

#include <iostream>

#include "eca_tlu_regs.h"

namespace eb_plugin {

ECA_TLU::ECA_TLU(etherbone::Device &dev) 
	: device(dev)
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

void ECA_TLU::configInput(unsigned channel,
	                      bool enable,
	                      uint64_t event,
	                      uint32_t stable)
{
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

} // namespace