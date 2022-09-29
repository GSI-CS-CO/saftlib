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

/* Includes */
/* ==================================================================================================== */
#include "SerdesClockGen.hpp"

/* Function CalcClockParameters(...) */
/* ==================================================================================================== */
void CalcClockParameters(double hi, double lo, uint64_t phase, struct SerClkGenControl *control)
{
  double period, wide_period, cut_period;
  int i, fill_factor, cut_wide_period;

#if IO_SER_CLK_GEN_DEBUG_MODE
  if (hi != floor(hi)) { fprintf(stderr, "warning: fractional part of hi time ignored; period remains unaffected\n"); }
#endif

  period = hi + lo;
  fill_factor = ceil(IO_SER_CLK_GEN_BITS/period);
  wide_period = period*fill_factor; // >= IO_SER_CLK_GEN_BITS
  cut_wide_period = floor(wide_period);
  cut_period = (double)cut_wide_period / fill_factor;
#if IO_SER_CLK_GEN_DEBUG_MODE
  fprintf(stderr, "wide_period     = %f\n", wide_period);
#endif

  control->period_integer  = floor(wide_period);
  control->period_high     = floor(hi);
  control->period_fraction = round((wide_period - control->period_integer) * 4294967296.0);
  control->phase_offset    = phase;
#if IO_SER_CLK_GEN_DEBUG_MODE
  fprintf(stderr, "period_integer   = 0x%x\n", control->period_integer);
  fprintf(stderr, "period_high      = 0x%x\n", control->period_high);
  fprintf(stderr, "period_fraction  = 0x%x\n", control->period_fraction);
  fprintf(stderr, "phase_offset     = 0x%lx\n", control->phase_offset);
#endif

  control->bit_pattern_normal = 0;
  for (i = 0; i < fill_factor; ++i) {
    int offset  = ceil(i*cut_period); // ceil guarantees gap before 7 is small
    if (IO_SER_CLK_GEN_BITS-1 >= offset) // future bits
      control->bit_pattern_normal |= 1 << (IO_SER_CLK_GEN_BITS-1 - offset);
    if (cut_wide_period - offset <= IO_SER_CLK_GEN_BITS) // past bits
      control->bit_pattern_normal |= 1 << (IO_SER_CLK_GEN_BITS-1 - offset + cut_wide_period);
  }

  // fractional bit insertion (0) must happen at cut_wide_period-1
  uint16_t spot = (cut_wide_period >= 16) ? 0 : (0x8000 >> (15-cut_wide_period));
  uint16_t low_mask = spot-1;
  //low_mask = 0xFF; // old approach
  uint16_t high_mask = ~low_mask;
  control->bit_pattern_skip = (control->bit_pattern_normal & high_mask) |
                              ((control->bit_pattern_normal & low_mask) >> 1);
#if IO_SER_CLK_GEN_DEBUG_MODE
  fprintf(stderr, "bit_pattern_norm = ");
  for (j = 15; j >= 0; --j)
    fprintf(stderr, "%d", (control->bit_pattern_normal >> j) & 1);
  fprintf(stderr, "\n");
  fprintf(stderr, "bit_pattern_skip = ");
  for (j = 15; j >= 0; --j)
    fprintf(stderr, "%d", (control->bit_pattern_skip >> j) & 1);
  fprintf(stderr, "\n");
#endif
}
