/** @file        eca_tlu_regs.h
  * DesignUnit   eca_tlu
  * @author      Wesley W. Terpstra <w.terpstra@gsi.de>
  * @date        15/04/2016
  * @version     2.0
  * @copyright   2016 GSI Helmholtz Centre for Heavy Ion Research GmbH
  *
  * @brief       Register map for Wishbone interface of VHDL entity <eca_tlu_auto>
  */

#ifndef _ECA_TLU_H_
#define _ECA_TLU_H_

  #define ECA_TLU_SDB_VENDOR_ID 0x00000651
  #define ECA_TLU_SDB_DEVICE_ID 0x7c82afbc

  #define ECA_TLU_NUM_INPUTS_GET   0x00  //ro,  8 b, Total number of inputs attached to the TLU
  #define ECA_TLU_INPUT_SELECT_RW  0x04  //rw,  8 b, Write the configuration of this input
  #define ECA_TLU_ENABLE_RW        0x08  //rw,  1 b, Will this input generate timing events on an edge
  #define ECA_TLU_STABLE_RW        0x0c  //rw, 32 b, Signal must be high/low for stable cycles to be counted as a valid transition
  #define ECA_TLU_EVENT_HI_RW      0x10  //rw, 32 b, Timing Event to generate (high word)
  #define ECA_TLU_EVENT_LO_RW      0x14  //rw, 32 b, Timing Event to generate (low word), lowest bit is replaced with the edge of the transition
  #define ECA_TLU_WRITE_OWR        0x18  //wo,  1 b, Write register contents to TLU configuration

#endif
