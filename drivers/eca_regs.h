/** @file        eca_regs.h
  * DesignUnit   eca
  * @author      Wesley W. Terpstra <w.terpstra@gsi.de>
  * @date        08/04/2016
  * @version     2.0
  * @copyright   2016 GSI Helmholtz Centre for Heavy Ion Research GmbH
  *
  * @brief       Register map for Wishbone interface of VHDL entity <eca_auto>
  */

#ifndef _ECA_H_
#define _ECA_H_

  #define ECA_SDB_VENDOR_ID 0x00000651
  #define ECA_SDB_DEVICE_ID 0xb2afc251

  #define ECA_CHANNELS_GET               0x00  //ro,  8 b, Number of channels implemented by the ECA, including the internal IO channel #0
  #define ECA_SEARCH_CAPACITY_GET        0x04  //ro, 16 b, Total number of search table entries per active page
  #define ECA_WALKER_CAPACITY_GET        0x08  //ro, 16 b, Total number of walker table entries per active page
  #define ECA_LATENCY_GET                0x0c  //ro, 32 b, Delay in ticks (typically nanoseconds) between an event's arrival at the ECA and its earliest possible execution as an action
  #define ECA_OFFSET_BITS_GET            0x10  //ro,  8 b, Actions scheduled for execution with a delay in ticks exceeding offset_bits are executed early
  #define ECA_FLIP_ACTIVE_OWR            0x14  //wo,  1 b, Flip the active search and walker tables with the inactive tables
  #define ECA_TIME_HI_GET                0x18  //ro, 32 b, Ticks (nanoseconds) since Jan 1, 1970 (high word)
  #define ECA_TIME_LO_GET                0x1c  //ro, 32 b, Ticks (nanoseconds) since Jan 1, 1970 (low word)
  #define ECA_SEARCH_SELECT_RW           0x20  //rw, 16 b, Read/write this record in the inactive search table
  #define ECA_SEARCH_RW_FIRST_RW         0x24  //rw, 16 b, Scratch register to be written to search_ro_first
  #define ECA_SEARCH_RW_EVENT_HI_RW      0x28  //rw, 32 b, Scratch register to be written to search_ro_event_hi
  #define ECA_SEARCH_RW_EVENT_LO_RW      0x2c  //rw, 32 b, Scratch register to be written to search_ro_event_lo
  #define ECA_SEARCH_WRITE_OWR           0x30  //wo,  1 b, Store the scratch registers to the inactive search table record search_select
  #define ECA_SEARCH_RO_FIRST_GET        0x34  //ro, 16 b, The first walker entry to execute if an event matches this record in the search table
  #define ECA_SEARCH_RO_EVENT_HI_GET     0x38  //ro, 32 b, Event IDs greater than or equal to this value match this search table record (high word)
  #define ECA_SEARCH_RO_EVENT_LO_GET     0x3c  //ro, 32 b, Event IDs greater than or equal to this value match this search table record (low word)
  #define ECA_WALKER_SELECT_RW           0x40  //rw, 16 b, Read/write this record in the inactive walker table
  #define ECA_WALKER_RW_NEXT_RW          0x44  //rw, 16 b, Scratch register to be written to walker_ro_next
  #define ECA_WALKER_RW_OFFSET_HI_RW     0x48  //rw, 32 b, Scratch register to be written to walker_ro_offset_hi
  #define ECA_WALKER_RW_OFFSET_LO_RW     0x4c  //rw, 32 b, Scratch register to be written to walker_ro_offset_lo
  #define ECA_WALKER_RW_TAG_RW           0x50  //rw, 32 b, Scratch register to be written to walker_ro_tag
  #define ECA_WALKER_RW_FLAGS_RW         0x54  //rw,  4 b, Scratch register to be written to walker_ro_flags
  #define ECA_WALKER_RW_CHANNEL_RW       0x58  //rw,  8 b, Scratch register to be written to walker_ro_channel
  #define ECA_WALKER_RW_NUM_RW           0x5c  //rw,  8 b, Scratch register to be written to walker_ro_num
  #define ECA_WALKER_WRITE_OWR           0x60  //wo,  1 b, Store the scratch registers to the inactive walker table record walker_select
  #define ECA_WALKER_RO_NEXT_GET         0x64  //ro, 16 b, The next walker entry to execute after this record (0xffff = end of list)
  #define ECA_WALKER_RO_OFFSET_HI_GET    0x68  //ro, 32 b, The resulting action's deadline is the event timestamp plus this offset (high word)
  #define ECA_WALKER_RO_OFFSET_LO_GET    0x6c  //ro, 32 b, The resulting action's deadline is the event timestamp plus this offset (low word)
  #define ECA_WALKER_RO_TAG_GET          0x70  //ro, 32 b, The resulting actions's tag
  #define ECA_WALKER_RO_FLAGS_GET        0x74  //ro,  4 b, Execute the resulting action even if it suffers from the errors set in this flag register
  #define ECA_WALKER_RO_CHANNEL_GET      0x78  //ro,  8 b, The channel to which the resulting action will be sent
  #define ECA_WALKER_RO_NUM_GET          0x7c  //ro,  8 b, The subchannel to which the resulting action will be sent
  #define ECA_CHANNEL_SELECT_RW          0x80  //rw,  8 b, Read/clear this channel
  #define ECA_CHANNEL_NUM_SELECT_RW      0x84  //rw,  8 b, Read/clear this subchannel
  #define ECA_CHANNEL_CODE_SELECT_RW     0x88  //rw,  2 b, Read/clear this error condition (0=late, 1=early, 2=conflict, 3=delayed)
  #define ECA_CHANNEL_TYPE_GET           0x90  //ro, 32 b, Type of the selected channel (0=io, 1=linux, 2=wbm, ...)
  #define ECA_CHANNEL_MAX_NUM_GET        0x94  //ro,  8 b, Total number of subchannels supported by the selected channel
  #define ECA_CHANNEL_CAPACITY_GET       0x98  //ro, 16 b, Total number of actions which may be enqueued by the selected channel at a time
  #define ECA_CHANNEL_MSI_SET_ENABLE_OWR 0x9c  //wo,  1 b, Turn on/off MSI messages for the selected channel
  #define ECA_CHANNEL_MSI_GET_ENABLE_GET 0xa0  //ro,  1 b, Check if MSI messages are enabled for the selected channel
  #define ECA_CHANNEL_MSI_SET_TARGET_OWR 0xa4  //wo, 32 b, Set the destination MSI address for the selected channel (only possible while it has MSIs disabled)
  #define ECA_CHANNEL_MSI_GET_TARGET_GET 0xa8  //ro, 32 b, Get the destination MSI address for the selected channel
  #define ECA_CHANNEL_MOSTFULL_ACK_GET   0xac  //ro, 32 b, Read the selected channel's fill status (used_now<<16 | used_most), MSI=(6<<16) will be sent if used_most changes
  #define ECA_CHANNEL_MOSTFULL_CLEAR_GET 0xb0  //ro, 32 b, Read and clear the selected channel's fill status (used_now<<16 | used_most), MSI=(6<<16) will be sent if used_most changes
  #define ECA_CHANNEL_VALID_COUNT_GET    0xb4  //ro, 32 b, Read and clear the number of actions output by the selected subchannel, MSI=(4<<16|num) will be sent when the count becomes non-zero
  #define ECA_CHANNEL_OVERFLOW_COUNT_GET 0xb8  //ro, 32 b, Read and clear the number of actions which could not be enqueued to the selected full channel which were destined for the selected subchannel, MSI=(5<<16|num) will be sent when the count becomes non-zero
  #define ECA_CHANNEL_FAILED_COUNT_GET   0xbc  //ro, 32 b, Read and clear the number of actions with the selected error code which were destined for the selected subchannel, MSI=(code<<16|num) will be sent when the count becomes non-zero
  #define ECA_CHANNEL_EVENT_ID_HI_GET    0xc0  //ro, 32 b, The event ID of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (high word)
  #define ECA_CHANNEL_EVENT_ID_LO_GET    0xc4  //ro, 32 b, The event ID of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (low word)
  #define ECA_CHANNEL_PARAM_HI_GET       0xc8  //ro, 32 b, The parameter of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (high word)
  #define ECA_CHANNEL_PARAM_LO_GET       0xcc  //ro, 32 b, The parameter of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (low word)
  #define ECA_CHANNEL_TAG_GET            0xd0  //ro, 32 b, The tag of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read
  #define ECA_CHANNEL_TEF_GET            0xd4  //ro, 32 b, The TEF of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read
  #define ECA_CHANNEL_DEADLINE_HI_GET    0xd8  //ro, 32 b, The deadline of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (high word)
  #define ECA_CHANNEL_DEADLINE_LO_GET    0xdc  //ro, 32 b, The deadline of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (low word)
  #define ECA_CHANNEL_EXECUTED_HI_GET    0xe0  //ro, 32 b, The actual execution time of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (high word)
  #define ECA_CHANNEL_EXECUTED_LO_GET    0xe4  //ro, 32 b, The actual execution time of the first action with the selected error code on the selected subchannel, cleared when channel_failed_count is read (low word)

#endif
