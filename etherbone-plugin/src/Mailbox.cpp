#include "Mailbox.hpp"

#include <saftbus/error.hpp>

#include <iostream>

#define MAILBOX_VENDOR_ID        0x651
#define MAILBOX_DEVICE_ID       0xfab0bdd8

namespace eb_plugin {

Mailbox::Mailbox(etherbone::Device &dev) 
	: device(dev)
{
	std::cerr << "Mailbox::Mailbox()" << std::endl;
	std::vector<sdb_device> mailbox_dev;
	device.sdb_find_by_identity(MAILBOX_VENDOR_ID, MAILBOX_DEVICE_ID, mailbox_dev);

	if (mailbox_dev.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "no mailbox device found on hardware");
	}
	if (mailbox_dev.size() > 1) {
		std::cerr << "more than one mailbox device found on hardware, taking the first one" << std::endl;
	}
	mailbox = static_cast<eb_address_t>(mailbox_dev[0].sdb_component.addr_first);


	std::vector<etherbone::sdb_msi_device> mailbox_msi;
	device.sdb_find_by_identity_msi(MAILBOX_VENDOR_ID, MAILBOX_DEVICE_ID, mailbox_msi);
	if (mailbox_msi.size() < 1) {
		throw saftbus::Error(saftbus::Error::FAILED, "mailbox has no MSI");
	}
	if (mailbox_msi.size() > 1) {
		std::cerr << "more than one mailbox MSI device found on hardware, taking the first one" << std::endl;
	}


	mailbox_msi_first = mailbox_msi[0].msi_first;

	std::cerr << "Mailbox msi_first = " << std::hex << mailbox_msi_first << std::endl;
}

int Mailbox::ConfigureSlot(uint32_t target_address) 
{
	std::cerr << "Mailbox::ConfigureSlot()" << std::endl;
	const auto num_slots = 128;
	eb_data_t mb_value;
	unsigned slot_index = 0;

	for (slot_index = 0; slot_index < num_slots; ++slot_index) {
		device.read(mailbox + slot_index * 4 * 2, EB_DATA32, &mb_value);
		if (mb_value == 0xffffffff) {
			break;
		}
	}

	if (slot_index < num_slots) {
		device.write(mailbox + slot_index * 4 * 2 + 4, EB_DATA32, (eb_data_t)target_address);
		return slot_index;
	}
	std::cerr << "no free mailbox slots " << std::endl;
	return -1;
}

void Mailbox::UseSlot(int slot_index, uint32_t value)
{
	std::cerr << "Mailbox::UseSlot()" << std::endl;
	device.write(mailbox + slot_index * 4 * 2, EB_DATA32, (eb_data_t)value);
}

void Mailbox::FreeSlot(int slot_index)
{
	device.write(mailbox + slot_index * 4 * 2 + 4, EB_DATA32, 0xffffffff);
}




} // namespace