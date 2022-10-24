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
#ifndef SER_CLK_GEN_REGS_H
#define SER_CLK_GEN_REGS_H

/* Includes */
/* ==================================================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>

/* Defines */
/* ==================================================================================================== */
#define IO_SER_CLK_GEN_PRODUCT_ID    0x5f3eaf43
#define IO_SER_CLK_GEN_VENDOR_ID     0x00000651

#define IO_SER_CLK_GEN_BITS          8

#define IO_SER_CLK_GEN_DEBUG_MODE    0

/* Functions */
/* ==================================================================================================== */
void CalcClockParameters(double hi, double lo, uint64_t phase, struct SerClkGenControl *control);

/* Enumerations */
/* ==================================================================================================== */
typedef enum
{
  eSCK_selr      = 0x00,
  eSCK_perr      = 0x04,
  eSCK_perhir    = 0x08,
  eSCK_fracr     = 0x0c,
  eSCK_normmaskr = 0x10,
  eSCK_skipmaskr = 0x14,
  eSCK_phofslr   = 0x18,
  eSCK_phofshr   = 0x1c,
} e_SerClkGen_RegisterArea;

/* Structures */
/* ==================================================================================================== */
typedef struct SerClkGenControl
{
  uint32_t period_integer;
  uint32_t period_high;
  uint32_t period_fraction;
  uint16_t bit_pattern_normal;
  uint16_t bit_pattern_skip;
  uint64_t phase_offset;
} s_SerClkGenControl;

#endif
