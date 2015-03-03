/** @file hw-eca.h
 *  @brief Register layout of the ECA unit.
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *
 *  Copyright (C) 2013 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  All offsets for regsiters in the ECA unit.
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

#ifndef ECA_HW_H
#define ECA_HW_H

#define GSI_VENDOR_ID	0x651
#define ECA_DEVICE_ID	0x8752bf44U
#define ECAE_DEVICE_ID	0x8752bf45U
#define ECAQ_DEVICE_ID	0x9bfa4560U

#define ECA_FEATURE_INSPECT_TABLE 0x1
#define ECA_FEATURE_INSPECT_QUEUE 0x2

#define ECA_CTL_DISABLE		0x01
#define ECA_CTL_INT_ENABLE	0x02
#define ECA_CTL_FLIP		0x04

#define ECA_INFO	0x00
#define ECA_TABLE_SIZE	0x00
#define ECA_QUEUE_SIZE	0x01
#define ECA_NUM_CHANNELS 0x02
#define ECA_INDEX	0x03
#define ECA_CTL		0x04
#define ECA_FEATURE	0x04
#define ECA_NAME	0x05
#define ECA_CTL_CLEAR	0x06
#define ECA_CTL_SET	0x07
#define ECA_TIME1	0x08
#define ECA_TIME0	0x0C
#define ECA_SEARCH	0x10
#define ECA_FIRST	0x14
#define ECA_EVENT1	0x18
#define ECA_EVENT0	0x1C
#define ECA_WALK	0x20
#define ECA_NEXT	0x24
#define ECA_DELAY1	0x28
#define ECA_DELAY0	0x2C
#define ECA_TAG		0x30
#define ECA_CHANNEL	0x34
#define ECA_FREQ_MUL	0x38
#define ECA_FREQ_5S	0x3C
#define ECA_FREQ_2S	0x3D
#define ECA_FREQ_DIV	0x3E

#define ECAC_STATUS_VALID	0x01
#define ECAC_STATUS_LATE	0x02

#define ECAC_CTL_DRAIN		0x01
#define ECAC_CTL_FREEZE		0x02
#define ECAC_CTL_INT_MASK	0x04

#define ECAC_SELECT	0x40
#define ECAC_CHANNEL	0x40
#define ECAC_INDEX	0x42
#define ECAC_CTL	0x44
#define ECAC_STATUS	0x44
#define ECAC_NAME	0x45
#define ECAC_CTL_CLEAR	0x46
#define ECAC_CTL_SET	0x47
#define ECAC_INT_DEST	0x48
#define ECAC_FILL	0x50
#define ECAC_MAX_FILL	0x52
#define ECAC_VALID	0x54
#define ECAC_CONFLICT	0x58
#define ECAC_LATE	0x5C

#define ECAC_EVENT1	0x60
#define ECAC_EVENT0	0x64
#define ECAC_PARAM1	0x68
#define ECAC_PARAM0	0x6C
#define ECAC_TAG	0x70
#define ECAC_TEF	0x74
#define ECAC_TIME1	0x78
#define ECAC_TIME0	0x7C

#define ECAQ_CTL	0x0
#define ECAQ_INT_MASK	0x4
#define ECAQ_ARRIVAL	0x8
#define ECAQ_OVERFLOW	0xC
#define ECAQ_QUEUED	0x10
#define ECAQ_DROPPED	0x14
#define ECAQ_META	0x18
#define ECAQ_FLAGS	0x1C

#define ECAQ_EVENT1	0x20
#define ECAQ_EVENT0	0x24
#define ECAQ_PARAM1	0x28
#define ECAQ_PARAM0	0x2C
#define ECAQ_TAG	0x30
#define ECAQ_TEF	0x34
#define ECAQ_TIME1	0x38
#define ECAQ_TIME0	0x3C

#endif
