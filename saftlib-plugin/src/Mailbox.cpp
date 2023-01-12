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

#include "Mailbox.hpp"

#include <saftbus/error.hpp>

#include <iostream>

#define MAILBOX_VENDOR_ID        0x651
#define MAILBOX_DEVICE_ID       0xfab0bdd8

namespace saftlib {

Mailbox::Mailbox(etherbone::Device &dev)
	: MsiDevice(dev, MAILBOX_VENDOR_ID, MAILBOX_DEVICE_ID) 
	, device(dev)
{
	mailbox = adr_first;
	mailbox_msi_first = msi_device.msi_first;
}

std::unique_ptr<Mailbox::Slot> Mailbox::ConfigureSlot(uint32_t target_address) 
{
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
		return std::unique_ptr<Mailbox::Slot>(new Mailbox::Slot(this, slot_index));
	}
	std::cerr << "no free mailbox slots " << std::endl;
	return std::unique_ptr<Mailbox::Slot>();
}

void Mailbox::UseSlot(int slot_index, uint32_t value)
{
	device.write(mailbox + slot_index * 4 * 2, EB_DATA32, (eb_data_t)value);
}

void Mailbox::FreeSlot(int slot_index)
{
	device.write(mailbox + slot_index * 4 * 2 + 4, EB_DATA32, 0xffffffff);
}

Mailbox::Slot::Slot(Mailbox *mailbox, int index) 
	: mb(mailbox)
	, slot_index(index) 
{
}
Mailbox::Slot::~Slot()
{
	mb->FreeSlot(slot_index);
}

int Mailbox::Slot::getIndex()
{
	return slot_index;
}


void Mailbox::Slot::Use(uint32_t value)
{
	mb->UseSlot(slot_index, value);
}




} // namespace