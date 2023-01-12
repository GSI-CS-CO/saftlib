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

#ifndef saftlib_MAILBOX_HPP_
#define saftlib_MAILBOX_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

#include <MsiDevice.hpp>

#include <memory>

namespace saftlib {

class Mailbox : public MsiDevice {
	etherbone::Device &device;
	eb_address_t mailbox;
	eb_address_t mailbox_msi_first;
	friend class Slot;
	/// @brief write a value to the preconfigured address.
	/// @param slot_index the slot to be used (return value of ConfigureSlot)
	/// @param value is the value to be written
	///
	void UseSlot(int slot_index, uint32_t value);
	/// @brief if a slot is no longer used, it should be marked as free by using this function
	/// @param slot_index the slot to be freed
	///
	void FreeSlot(int slot_index);

public:
	class Slot {
		Mailbox *mb;
		int slot_index;
		Slot(Mailbox *mailbox, int index);
		friend class Mailbox;
	public:
		/// @brief free Mailbox hardware ressources for that slot
		~Slot();

		/// @brief the slot index that is owned
		/// @return slot index
		int getIndex();

		/// @brief write a value to the preconfigured address.
		/// @param slot_index the slot to be used (return value of ConfigureSlot)
		/// @param value is the value to be written
		///
		void Use(uint32_t value);
	};

	Mailbox(etherbone::Device &device);
	/// @brief find a free slot in the mailbox and configure it with target_address
	/// @param target_address specifies to which address the value in UseSlot will be written
	/// @return the slot number which was used, -1 if no free slot was found
	///
	std::unique_ptr<Slot> ConfigureSlot(uint32_t target_address);
};

}

#endif
