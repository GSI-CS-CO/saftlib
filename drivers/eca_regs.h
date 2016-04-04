/** @file        eca_regs.h
  * DesignUnit   eca
  * @author      Wesley W. Terpstra <w.terpstra@gsi.de>
  * @date        01/04/2016
  * @version     2.0
  * @copyright   2016 GSI Helmholtz Centre for Heavy Ion Research GmbH
  *
  * @brief       Register map for Wishbone interface of VHDL entity <eca_auto>
  */

#ifndef _ECA_H_
#define _ECA_H_

   #define ECA_SDB_VENDOR_ID 0x00000651
   #define ECA_SDB_DEVICE_ID 0xb2afc251

   #define ECA_CHANNELS_GET                0x00  //ro,  8 b, 
   #define ECA_SEARCH_CAPACITY_GET         0x04  //ro, 16 b, 
   #define ECA_WALKER_CAPACITY_GET         0x08  //ro, 16 b, 
   #define ECA_LATENCY_GET                 0x0c  //ro, 32 b, 
   #define ECA_OFFSET_BITS_GET             0x10  //ro,  8 b, 
   #define ECA_FLIP_ACTIVE_OWR             0x14  //wo,  1 b, 
   #define ECA_TIME_HI_GET                 0x18  //ro, 32 b, 
   #define ECA_TIME_LO_GET                 0x1c  //ro, 32 b, 
   #define ECA_SEARCH_SELECT_RW            0x20  //rw, 16 b, 
   #define ECA_SEARCH_RO_FIRST_GET         0x24  //ro, 16 b, 
   #define ECA_SEARCH_RO_EVENT_HI_GET      0x28  //ro, 32 b, 
   #define ECA_SEARCH_RO_EVENT_LO_GET      0x2c  //ro, 32 b, 
   #define ECA_SEARCH_WRITE_OWR            0x30  //wo,  1 b, 
   #define ECA_SEARCH_RW_FIRST_RW          0x34  //rw, 16 b, 
   #define ECA_SEARCH_RW_EVENT_HI_RW       0x38  //rw, 32 b, 
   #define ECA_SEARCH_RW_EVENT_LO_RW       0x3c  //rw, 32 b, 
   #define ECA_WALKER_SELECT_RW            0x40  //rw, 16 b, 
   #define ECA_WALKER_RO_NEXT_GET          0x44  //ro, 16 b, 
   #define ECA_WALKER_RO_OFFSET_HI_GET     0x48  //ro, 32 b, 
   #define ECA_WALKER_RO_OFFSET_LO_GET     0x4c  //ro, 32 b, 
   #define ECA_WALKER_RO_TAG_GET           0x50  //ro, 32 b, 
   #define ECA_WALKER_RO_FLAGS_GET         0x54  //ro,  4 b, 
   #define ECA_WALKER_RO_CHANNEL_GET       0x58  //ro,  8 b, 
   #define ECA_WALKER_RO_NUM_GET           0x5c  //ro,  8 b, 
   #define ECA_WALKER_WRITE_OWR            0x60  //wo,  1 b, 
   #define ECA_WALKER_RW_NEXT_RW           0x64  //rw, 16 b, 
   #define ECA_WALKER_RW_OFFSET_HI_RW      0x68  //rw, 32 b, 
   #define ECA_WALKER_RW_OFFSET_LO_RW      0x6c  //rw, 32 b, 
   #define ECA_WALKER_RW_TAG_RW            0x70  //rw, 32 b, 
   #define ECA_WALKER_RW_FLAGS_RW          0x74  //rw,  4 b, 
   #define ECA_WALKER_RW_CHANNEL_RW        0x78  //rw,  8 b, 
   #define ECA_WALKER_RW_NUM_RW            0x7c  //rw,  8 b, 
   #define ECA_CHANNEL_SELECT_RW           0x80  //rw,  8 b, 
   #define ECA_CHANNEL_NUM_SELECT_RW       0x84  //rw,  8 b, 
   #define ECA_CHANNEL_CODE_SELECT_RW      0x88  //rw,  2 b, 
   #define ECA_CHANNEL_TYPE_GET            0x8c  //ro, 32 b, 
   #define ECA_CHANNEL_MAX_NUM_GET         0x90  //ro,  8 b, 
   #define ECA_CHANNEL_CAPACITY_GET        0x94  //ro, 16 b, 
   #define ECA_CHANNEL_MSI_SET_ENABLE_OWR  0x98  //wo,  1 b, 
   #define ECA_CHANNEL_MSI_GET_ENABLE_GET  0x9c  //ro,  1 b, 
   #define ECA_CHANNEL_MSI_SET_TARGET_OWR  0xa0  //wo, 32 b, 
   #define ECA_CHANNEL_MSI_GET_TARGET_GET  0xa4  //ro, 32 b, 
   #define ECA_CHANNEL_OVERFLOW_COUNT_GET  0xa8  //ro, 32 b, 
   #define ECA_CHANNEL_MOSTFULL_ACK_GET    0xac  //ro, 32 b, 
   #define ECA_CHANNEL_MOSTFULL_CLEAR_GET  0xb0  //ro, 32 b, 
   #define ECA_CHANNEL_VALID_COUNT_GET     0xb4  //ro, 32 b, 
   #define ECA_CHANNEL_FAILED_COUNT_GET    0xb8  //ro, 32 b, 
   #define ECA_CHANNEL_EVENT_ID_HI_GET     0xbc  //ro, 32 b, 
   #define ECA_CHANNEL_EVENT_ID_LO_GET     0xc0  //ro, 32 b, 
   #define ECA_CHANNEL_PARAM_HI_GET        0xc4  //ro, 32 b, 
   #define ECA_CHANNEL_PARAM_LO_GET        0xc8  //ro, 32 b, 
   #define ECA_CHANNEL_TAG_GET             0xcc  //ro, 32 b, 
   #define ECA_CHANNEL_TEF_GET             0xd0  //ro, 32 b, 
   #define ECA_CHANNEL_DEADLINE_HI_GET     0xd4  //ro, 32 b, 
   #define ECA_CHANNEL_DEADLINE_LO_GET     0xd8  //ro, 32 b, 
   #define ECA_CHANNEL_EXECUTED_HI_GET     0xdc  //ro, 32 b, 
   #define ECA_CHANNEL_EXECUTED_LO_GET     0xe0  //ro, 32 b, 

#endif
