#ifndef saftlib_MAILBOX_HPP_
#define saftlib_MAILBOX_HPP_

#ifndef ETHERBONE_THROWS
#define ETHERBONE_THROWS 1
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <etherbone.h>

namespace saftlib {

class Mailbox {
	etherbone::Device &device;
	eb_address_t mailbox;
	eb_address_t mailbox_msi_first;
public:
	Mailbox(etherbone::Device &device);
	/// @brief find a free slot in the mailbox and configure it with target_address
	/// @param target_address specifies to which address the value in UseSlot will be written
	/// @return the slot number which was used, -1 if no free slot was found
	///
	// @saftbus-export
	int ConfigureSlot(uint32_t target_address);

	/// @brief write a value to the preconfigured address.
	/// @param slot_index the slot to be used (return value of ConfigureSlot)
	/// @param value is the value to be written
	///
	// @saftbus-export
	void UseSlot(int slot_index, uint32_t value);

	/// @brief if a slot is no longer used, it should be marked as free by using this function
	/// @param slot_index the slot to be freed
	///
	// @saftbus-export
	void FreeSlot(int slot_index);

	eb_address_t get_msi_first() {return mailbox_msi_first; }
};

}

#endif
