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
#ifndef IOCONTROL_REGS_H
#define IOCONTROL_REGS_H

/* Defines */
/* ==================================================================================================== */
#define IO_CONTROL_PRODUCT_ID        0x10c05791
#define IO_CONTROL_VENDOR_ID         0x00000651

#define IO_MAX_IOS_PER_CHANNEL       64 /* 64 IOs per channel */
#define IO_MAX_VALID_CHANNELS        2  /* GPIO and LVDS */

#define IO_LVDS_MAX                  64
#define IO_GPIO_MAX                  64
#define IO_FIXED_MAX                 64

#define IO_LEGACY_MODE_ENABLE        0x1
#define IO_LEGACY_MODE_DISABLE       0x0

#define IO_INFO_INOUT_COUNT_MASK     0x000000ff
#define IO_INFO_OUT_COUNT_MASK       0x0000ff00
#define IO_INFO_IN_COUNT_MASK        0x00ff0000
#define IO_INFO_TOTAL_COUNT_MASK     0xff000000
#define IO_INFO_INOUT_SHIFT          0
#define IO_INFO_OUT_SHIFT            8
#define IO_INFO_IN_SHIFT             16
#define IO_INFO_TOTAL_SHIFT          24

#define IO_FIELD_PARAM_INVALID       0xffffffff
#define IO_FIELD_NUNBER_MASK         0xff000000
#define IO_FIELD_INTERNAL_ID_MASK    0x00ff0000
#define IO_FIELD_CFG_MASK            0x0000ff00
#define IO_FIELD_LOGIC_RES_MASK      0x000000ff
#define IO_FIELD_NUNBER_SHIFT        24
#define IO_FIELD_INTERNAL_ID_SHIFT   16
#define IO_FIELD_CFG_SHIFT           8
#define IO_FIELD_LOGIC_RES_SHIFT     0

#define IO_SPECIAL_PURPOSE_MASK      0xfc
#define IO_SPECIAL_OUT_MASK          0x02
#define IO_SPECIAL_IN_MASK           0x01

#define IO_SPECIAL_PURPOSE_SHIFT     2
#define IO_SPECIAL_OUT_SHIFT         1
#define IO_SPECIAL_IN_SHIFT          0

#define IO_CFG_FIELD_DIR_MASK        0xc0
#define IO_CFG_FIELD_INFO_CHAN_MASK  0x38
#define IO_CFG_FIELD_OE_MASK         0x04
#define IO_CFG_FIELD_TERM_MASK       0x02
#define IO_CFG_FIELD_RES_BIT_MASK    0x01

#define IO_CFG_FIELD_DIR_SHIFT       6
#define IO_CFG_FIELD_INFO_CHAN_SHIFT 3
#define IO_CFG_FIELD_OE_SHIFT        2
#define IO_CFG_FIELD_TERM_SHIFT      1
#define IO_CFG_FIELD_RES_BIT_SHIFT   0

#define IO_LOGIC_RES_FIELD_LL_MASK   0xf0
#define IO_LOGIC_RES_FIELD_RES_MASK  0x0f

#define IO_LOGIC_RES_FIELD_LL_SHIFT  4
#define IO_LOGIC_RES_FIELD_RES_SHIFT 0

#define IO_CFG_FIELD_DIR_OUTPUT      0
#define IO_CFG_FIELD_DIR_INPUT       1
#define IO_CFG_FIELD_DIR_INOUT       2

#define IO_CFG_OE_UNAVAILABLE        0
#define IO_CFG_OE_AVAILABLE          1

#define IO_CFG_TERM_UNAVAILABLE      0
#define IO_CFG_TERM_AVAILABLE        1

#define IO_CFG_SPEC_UNAVAILABLE      0
#define IO_CFG_SPEC_AVAILABLE        1

#define IO_CFG_CHANNEL_GPIO          0
#define IO_CFG_CHANNEL_LVDS          1
#define IO_CFG_CHANNEL_FIXED         2

#define IO_LOGIC_LEVEL_TTL           0
#define IO_LOGIC_LEVEL_LVTTL         1
#define IO_LOGIC_LEVEL_LVDS          2
#define IO_LOGIC_LEVEL_NIM           3
#define IO_LOGIC_LEVEL_CMOS          4

#define IO_OPERATION_SET             0
#define IO_OPERATION_RESET           1
#define IO_OPERATION_GET             2

#define IO_TYPE_OE                   0
#define IO_TYPE_TERM                 1
#define IO_TYPE_SPEC_IN              2
#define IO_TYPE_SPEC_OUT             3
#define IO_TYPE_MUX                  4
#define IO_TYPE_SEL                  5

#define __IO_RETURN_FAILURE          1
#define __IO_RETURN_SUCCESS          0
#define __IO_RETURN_IO_NAME_UNKNOWN  0xffffffff
#define __IO_RETURN_IO_OPTION_INVAL  0xfffffffe

/* Enumerations */
/* ==================================================================================================== */
typedef enum
{
  eGPIO_Oe_Legacy_low        = 0x0000,
  eLVDS_Oe_Legacy_low        = 0x0004,
  eGPIO_Oe_Legacy_high       = 0x0008,
  eLVDS_Oe_Legacy_high       = 0x000c,
  eIO_Config                 = 0x0010,
  eIO_Version                = 0x0100,
  eGPIO_Info                 = 0x0104,
  eLVDS_Info                 = 0x0108,
  eFIXED_Info                = 0x010c,
  eGPIO_Oe_Set_low           = 0x0200,
  eGPIO_Oe_Set_high          = 0x0204,
  eGPIO_Oe_Reset_low         = 0x0208,
  eGPIO_Oe_Reset_high        = 0x020c,
  eLVDS_Oe_Set_low           = 0x0300,
  eLVDS_Oe_Set_high          = 0x0304,
  eLVDS_Oe_Reset_low         = 0x0308,
  eLVDS_Oe_Reset_high        = 0x030c,
  eGPIO_Term_Set_low         = 0x0400,
  eGPIO_Term_Set_high        = 0x0404,
  eGPIO_Term_Reset_low       = 0x0408,
  eGPIO_Term_Reset_high      = 0x040c,
  eLVDS_Term_Set_low         = 0x0500,
  eLVDS_Term_Set_high        = 0x0504,
  eLVDS_Term_Reset_low       = 0x0508,
  eLVDS_Term_Reset_high      = 0x050c,
  eGPIO_Spec_In_Set_low      = 0x0600,
  eGPIO_Spec_In_Set_high     = 0x0604,
  eGPIO_Spec_In_Reset_low    = 0x0608,
  eGPIO_Spec_In_Reset_high   = 0x060c,
  eGPIO_Spec_Out_Set_low     = 0x0700,
  eGPIO_Spec_Out_Set_high    = 0x0704,
  eGPIO_Spec_Out_Reset_low   = 0x0708,
  eGPIO_Spec_Out_Reset_high  = 0x070c,
  eLVDS_Spec_In_Set_low      = 0x0800,
  eLVDS_Spec_In_Set_high     = 0x0804,
  eLVDS_Spec_In_Reset_low    = 0x0808,
  eLVDS_Spec_In_Reset_high   = 0x080c,
  eLVDS_Spec_Out_Set_low     = 0x0900,
  eLVDS_Spec_Out_Set_high    = 0x0904,
  eLVDS_Spec_Out_Reset_low   = 0x0908,
  eLVDS_Spec_Out_Reset_high  = 0x090c,
  eGPIO_Mux_Set_low          = 0x0a00,
  eGPIO_Mux_Set_high         = 0x0a04,
  eGPIO_Mux_Reset_low        = 0x0a08,
  eGPIO_Mux_Reset_high       = 0x0a0c,
  eLVDS_Mux_Set_low          = 0x0b00,
  eLVDS_Mux_Set_high         = 0x0b04,
  eLVDS_Mux_Reset_low        = 0x0b08,
  eLVDS_Mux_Reset_high       = 0x0b0c,
  eGPIO_Sel_Set_low          = 0x0c00,
  eGPIO_Sel_Set_high         = 0x0c04,
  eGPIO_Sel_Reset_low        = 0x0c08,
  eGPIO_Sel_Reset_high       = 0x0c0c,
  eLVDS_Sel_Set_low          = 0x0d00,
  eLVDS_Sel_Set_high         = 0x0d04,
  eLVDS_Sel_Reset_low        = 0x0d08,
  eLVDS_Sel_Reset_high       = 0x0d0c,
  eGPIO_In_Gate_Set_low      = 0x1000,
  eGPIO_In_Gate_Set_high     = 0x1004,
  eGPIO_In_Gate_Reset_low    = 0x1008,
  eGPIO_In_Gate_Reset_high   = 0x100c,
  eLVDS_In_Gate_Set_low      = 0x2000,
  eLVDS_In_Gate_Set_high     = 0x2004,
  eLVDS_In_Gate_Reset_low    = 0x2008,
  eLVDS_In_Gate_Reset_high   = 0x200c,
  eSet_GPIO_Out_Begin        = 0xa000,
  eSet_LVDS_Out_Begin        = 0xb000,
  eGet_GPIO_In_Begin         = 0xc000,
  eGet_LVDS_In_Begin         = 0xd000,
  eGPIO_PPS_Mux_Set_low      = 0x0e00,
  eGPIO_PPS_Mux_Set_high     = 0x0e04,
  eGPIO_PPS_Mux_Reset_low    = 0x0e08,
  eGPIO_PPS_Mux_Reset_high   = 0x0e0c,
  eLVDS_PPS_Mux_Set_low      = 0x0f00,
  eLVDS_PPS_Mux_Set_high     = 0x0f04,
  eLVDS_PPS_Mux_Reset_low    = 0x0f08,
  eLVDS_PPS_Mux_Reset_high   = 0x0f0c,
  eIO_Map_Table_Begin        = 0xe000
} e_IOCONTROL_RegisterArea;

/* Structures */
/* ==================================================================================================== */
typedef struct
{
  char          uName[12];
  unsigned char uSpecial;
  unsigned char uIndex;
  unsigned char uIOCfgSpace;
  unsigned char uLogicLevelRes;
} s_IOCONTROL_SetupField;

#endif
