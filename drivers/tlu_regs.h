/** Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#ifndef _TLU_H_
#define _TLU_H_

#define TLU_SLAVE_VENDOR_ID 0x0000000000000651ull
#define TLU_SLAVE_DEVICE_ID 0x10051981

//| Address Map ------------------------ slave ----------------------------------------------|
#define SLAVE_STAT_GET           0x00  // r  _0xffffffff , fifo n..0 status (0 empty, 1 ne)
#define SLAVE_CLEAR_SET          0x04  // w  _0xffffffff , Clear channels n..0
#define SLAVE_TEST_OWR           0x08  // w  _0xffffffff , trigger n..0 test
#define SLAVE_ACT_GET            0x0c  // rw _0xffffffff , Activate/Deactivate trigger n..0
#define SLAVE_ACT_SET            0x10  // rw _0xffffffff , ""
#define SLAVE_ACT_CLR            0x14  // rw _0xffffffff , ""
#define SLAVE_EDG_GET            0x18  // rw _0xffffffff , trigger n..0 latch edge (1 pos, 0 neg)
#define SLAVE_EDG_SET            0x1c  // rw _0xffffffff , ""
#define SLAVE_EDG_CLR            0x20  // rw _0xffffffff , ""
#define SLAVE_IE_RW              0x24  // rw _0x00000001 , Global IRQ enable
#define SLAVE_MSK_GET            0x28  // rw _0xffffffff , IRQ channels mask n..0
#define SLAVE_MSK_SET            0x2c  // rw _0xffffffff , ""
#define SLAVE_MSK_CLR            0x30  // rw _0xffffffff , ""
#define SLAVE_CH_NUM_GET         0x34  // r  _0x0000ffff , number channels present
#define SLAVE_CH_DEPTH_GET       0x38  // r  _0x0000ffff , channels depth
#define SLAVE_TC_GET_0           0x50  // r  _0xffffffff , Current time Cycle Count Hi (32b), Cycle Lo(32b)
#define SLAVE_TC_GET_1           0x54  // r  _0xffffffff , ""
#define SLAVE_CH_SEL_RW          0x58  // rw _0x0000001f , channels select register
#define SLAVE_TS_POP_OWR         0x5c  // wp _0x00000001 , writing anything here will pop selected channel
#define SLAVE_TS_TEST_OWR        0x60  // wp _0x000000ff , Test selected channel
#define SLAVE_TS_CNT_GET         0x64  // r  _0xffffffff , fifo fill count
#define SLAVE_TS_GET_0           0x68  // r  _0xffffffff , fifo q - Cycle Count Hi (32b), Cycle Lo(32b), Sub cycle word (8b) 
#define SLAVE_TS_GET_1           0x6c  // r  _0xffffffff , ""
#define SLAVE_TS_GET_2           0x70  // r  _0x000000ff , ""
#define SLAVE_STABLE_RW          0x74  // rw _0x0000ffff , stable time in ns, how long a signal has to be constant before edges are detected
#define SLAVE_TS_MSG_RW          0x78  // rw _0xffffffff , MSI msg to be sent 
#define SLAVE_TS_DST_ADR_RW      0x7c  // rw _0xffffffff , MSI adr to send to

#endif
