#include "OpenDevice.hpp"
#include "Mailbox.hpp"
#include "SAFTd.hpp"
#include "eb-forward.hpp"

#include <saftbus/error.hpp>

#include <iostream>
#include <functional>
#include <cassert>

namespace eb_plugin {

const eb_data_t MSI_TEST_VALUE = 0x12345678;

void OpenDevice::check_msi_callback(eb_data_t value) 
{
	assert(value == MSI_TEST_VALUE);
	check_msi_phase = false;
	mbox->FreeSlot(slot_idx);
	saftd->release_irq(irq_adr);
	mbox.reset();
}
bool OpenDevice::poll_msi(bool only_once) {
	std::cerr << "OpenDevice::poll_msi" << std::endl;
	etherbone::Cycle cycle;
	eb_data_t msi_adr = 0;
	eb_data_t msi_dat = 0;
	eb_data_t msi_cnt = 0;
	bool found_msi = false;
	const int MAX_MSIS_IN_ONE_GO = 5; // not too many MSIs at once to not block event loop for too long 
	for (int i = 0; i < MAX_MSIS_IN_ONE_GO; ++i) { // never more this many MSIs in one go
		cycle.open(device);
		cycle.read_config(0x40, EB_DATA32, &msi_adr);
		cycle.read_config(0x44, EB_DATA32, &msi_dat);
		cycle.read_config(0x48, EB_DATA32, &msi_cnt);
		cycle.close();
		if (msi_cnt & 1) {
			msi_adr -= msi_first;
			needs_polling = true; // this value is 
			found_msi = true;
			saftd->write(msi_adr, EB_DATA32, msi_dat); // this functon is normally called by etherbone::Socket when it receives an MSI
		}
		if (!(msi_cnt & 2)) { // no more msi to poll (second bit of msi_cnt is not set)
			break; 
		}
	}
	if ((msi_cnt & 2) || found_msi) {
		// if we polled MAX_MSIS_IN_ONE_GO but there are more MSIs (second bit of msi_cnt is set)
		// OR if there was at least one MSI present 
		// we have to schedule the next check immediately because the 
		// MSI we just polled may cause actions that trigger other MSIs.
		// however, this TimeoutSource will be called only once, because the only_once argument is true
		bool only_once;
		std::cerr << "add an only_once timeout source" << std::endl;
		saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
				std::bind(&OpenDevice::poll_msi, this, only_once=true), std::chrono::milliseconds(0), std::chrono::milliseconds(0)
			);
	} 

	if (only_once) {
		std::cerr << "polled only_once " << found_msi << std::endl;
		// returning false removes the TimeoutSource from the event loop
		return false;
	}
	if (!check_msi_phase && !needs_polling) {
		// return false if checking phase is over, and we found out that no polling is needed 
		return false;
	}
	return true;
}

OpenDevice::OpenDevice(const etherbone::Socket &socket, const std::string& eb_path, int polling_iv_ms, SAFTd *sd)
	: etherbone_path(eb_path), eb_forward_path(eb_path), polling_interval_ms(polling_iv_ms), saftd(sd), check_msi_phase(true), needs_polling(false) 
{
	std::cerr << "OpenDevice::OpenDevice(\"" << eb_path << "\")" << std::endl;
	device.open(socket, etherbone_path.c_str());
	stat(etherbone_path.c_str(), &dev_stat);

	// Use the mailbox device to send us an MSI. 
	// We need to find out if it arrives as "real" MSI or if
	// it needs to be polled from eb-slave config registers.
	try {
		if (saftd != nullptr) {
			device.enable_msi(&first, &last);
			mask = last - first;
			mbox = std::unique_ptr<Mailbox>(new Mailbox(device));
			msi_first = mbox->get_msi_first();
			std::cerr << "first=" << std::hex << first << " last=" << last << " msi_first=" << msi_first << std::endl;
			for (;;) {
				irq_adr = ((rand() & mask) + first) & (~0x3);
				std::cerr << "try to attach irq " << std::hex << irq_adr << std::endl;
				if (saftd->request_irq(irq_adr, std::bind(&OpenDevice::check_msi_callback, this, std::placeholders::_1))) {
					break;
				}
			}
			slot_idx = mbox->ConfigureSlot(irq_adr + msi_first);
			mbox->UseSlot(slot_idx, MSI_TEST_VALUE); // make one single irq that should call our check_msi_callback
			bool only_once;
			poll_timeout_source = saftbus::Loop::get_default().connect<saftbus::TimeoutSource>(
					std::bind(&OpenDevice::poll_msi, this, only_once=false), 
					std::chrono::milliseconds(polling_interval_ms),
					std::chrono::milliseconds(polling_interval_ms)
				);
		}

		// assume that only the /dev/ttyUSB<n> devices and /dev/pts/<n> devices need eb-forwarding
		if (eb_path.find("/ttyUSB") != eb_path.npos || eb_path.find("/pts/") != eb_path.npos ) {
			std::cerr << "create forwarding device" << std::endl;
			eb_forward = std::unique_ptr<EB_Forward>(new EB_Forward(eb_path));
			eb_forward_path = eb_forward->eb_forward_path();
		}

	} catch (saftbus::Error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}
OpenDevice::~OpenDevice()
{
	saftbus::Loop::get_default().remove(poll_timeout_source);
	chmod(etherbone_path.c_str(), dev_stat.st_mode);
	device.close();
}

std::string OpenDevice::getEtherbonePath() const
{
	return etherbone_path;
}

std::string OpenDevice::getEbForwardPath() const
{
	return eb_forward_path;
}


} // namespace
