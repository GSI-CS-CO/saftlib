/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
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
#ifndef FG_REGS_H
#define FG_REGS_H

// Locate shared memory of processors
#define LM32_RAM_USER_VENDOR      0x651       //vendor ID
#define LM32_RAM_USER_PRODUCT     0x54111351  //product ID
#define LM32_RAM_USER_VMAJOR      1           //major revision
#define LM32_RAM_USER_VMINOR      1           //minor revision

#define LM32_CLUSTER_ROM_VENDOR   0x651
#define LM32_CLUSTER_ROM_PRODUCT  0x10040086

#define MSI_MAILBOX_VENDOR        0x651
#define MSI_MAILBOX_PRODUCT       0xfab0bdd8

// Shared memory base address
#define SHM_BASE          0x500
#define BOARD_ID          0x0   // 64-bit
#define EXT_ID            0x8   // 64-bit
#define BACKPLANE_ID      0x10  // 64-bit
#define BOARD_TEMP        0x18  // 32-bit ...
#define EXT_TEMP          0x1c
#define BACKPLACE_TEMP    0x20
#define FG_MAGIC_NUMBER   0x24  // 0xdeadbeef
#define FG_VERSION        0x28  // expect v3
#define FG_MB_SLOT        0x2c  // mb slot for host=>lm32
#define FG_NUM_CHANNELS   0x30
#define FG_BUFFER_SIZE    0x34
#define FG_MACROS         0x38  // 256 entries of (hi..lo): slot, device, version, output-bits
#define FG_SCAN_DONE      0x100c0  // firmware writes 0 here if scan of channels is done

#define FG_MACROS_SIZE  256

#define FG_REGS_BASE_   0x438   // FG_MACROS + 4 * 256 + 4
#define FG_WPTR         0x0
#define FG_RPTR         0x4
#define FG_MBX_SLOT     0x8
#define FG_MACRO_NUM    0xc
#define FG_RAMP_COUNT   0x10
#define FG_TAG          0x14
#define FG_STATE        0x18
#define FG_REGS_SIZE    0x1c

// channel, num_channels
#define FG_REGS_BASE(c,n) (FG_REGS_BASE_+FG_REGS_SIZE*c)

#define PARAM_COEFF_AB  0x0 // high = coeff_a
#define PARAM_COEFF_C   0x4
#define PARAM_CONTROL   0x8   // 2..0=step, 5..3=freq, 11..6=shift_b, 17..12=shifta
#define PARAM_SIZE      0xc

// channel, index, num_channels, buffer_size
#define FG_BUFF_BASE(c,i,n,s) (FG_REGS_BASE(n,n)+(i+c*s)*PARAM_SIZE)


// Software interrupt numbers; host=>lm32
#define SWI_ENABLE  0x00020000UL
#define SWI_DISABLE 0x00030000UL
#define SWI_SCAN    0x00040000UL // cause firmware to scan all busses for fg channels

// interrupt payload; lm32=>host
#define IRQ_DAT_REFILL          0
#define IRQ_DAT_START           1
#define IRQ_DAT_STOP_EMPTY      2
#define IRQ_DAT_STOP_NOT_EMPTY  3
#define IRQ_DAT_ARMED           4
#define IRQ_DAT_DISARMED        5


#endif
