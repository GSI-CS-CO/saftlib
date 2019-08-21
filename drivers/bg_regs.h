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
#ifndef BG_REGS_H
#define BG_REGS_H

// Locate shared memory of processors
#define LM32_RAM_USER_VENDOR      0x651       //vendor ID
#define LM32_RAM_USER_PRODUCT     0x54111351  //product ID
#define LM32_RAM_USER_VMAJOR      1           //major revision
#define LM32_RAM_USER_VMINOR      1           //minor revision

#define LM32_CLUSTER_ROM_VENDOR   0x651
#define LM32_CLUSTER_ROM_PRODUCT  0x10040086

#define MSI_MAILBOX_VENDOR        0x651
#define MSI_MAILBOX_PRODUCT       0xfab0bdd8
#define MB_SLOT_RANGE             128          // 0 .. 127
#define MB_SLOT_CFG_FREE          0xffffffffUL

// defines
#define BG_FW_ID                  0xb2b2b2b2

// definitions of buffers in shared memory
#define SHM_BASE                  0x500

#define SHM_FW_ID                 SHM_BASE
#define SHM_MB_SLOT               SHM_BASE + 0x04UL
#define SHM_MB_SLOT_HOST          SHM_BASE + 0x0CUL
#define SHM_CMD                   SHM_BASE + 0x10UL
#define SHM_INPUT                 SHM_BASE + 0x20UL

#define EVT_ID_IO_H32             0x0000FCA0UL  // event id of timing message for IO actions (hi32)
#define EVT_ID_IO_L32             0x00000000UL  // event id of timing message for IO actions (lo32)
#define EVT_MASK_IO               0xFFFFFFFF00000000ULL

// user commands for the burst generator
#define CMD_SHOW_ALL              0x1UL         // show pulse parameters, pulse cycles
#define CMD_GET_PARAM             0x2UL         // get pulse parameters
#define CMD_GET_CYCLE             0x3UL         // get pulse cycles
#define CMD_LS_BURST              0x4UL         // list burst (burst ids or burst info)
#define CMD_MK_BURST              0x5UL         // declare new burst
#define CMD_RM_BURST              0x6UL         // remove burst
#define CMD_DE_BURST              0x7UL         // dis/enable burst
#define CMD_RD_MSI_ECPU           0x10UL        // read and show the content of ECA MSI registers (MSI enable, MSI destination address)
#define CMD_RD_ECPU_CHAN          0x11UL        // read and show the content of ECA counters for the eCPU action channel
#define CMD_RD_ECPU_QUEUE         0x12UL        // read and show the content of ECA queue connected to the eCPU action channel
#define CMD_LS_FW_ID              0x13UL        // list the firmware id (the value is written to the shared input buffer)

#define CTL_DIS                   0x0000UL
#define CTL_EN                    0x0001UL
#define CTL_VALID                 0x8000UL

#define N_BURSTS                  16            // maximum number of bursts
#define N_BURST_INFO              10            // the length of burst info (id, io_type, io_idx, start_h32/l32, stop_h32/l32, cycle_h32/l32, flag)
#endif
