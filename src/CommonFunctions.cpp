/*  Copyright (C) 2011-2016, 2021-2022 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *          Michael Reese <m.reese@gsi.de>
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

#include "CommonFunctions.hpp"

#include <saftbus/client.hpp>

using namespace std;

/* Format mask for action sink */
uint64_t tr_mask(int i)
{
  return i ? (((uint64_t)-1) << (64-i)) : 0;
} //tr_mask

//
std::string tr_formatDate(saftlib::Time time, uint32_t pmode, bool json)
{
  uint64_t t     = time.getTAI();
  if (pmode & PMODE_UTC) {
    t = time.getUTC();
  }
  uint64_t ns    = t % 1000000000;
  time_t   s     = t / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  char full[80];
  char utc[10] = "";
  string temp;

  if (pmode & PMODE_UTC) {
    if (time.isLeapUTC()) {
      snprintf(utc, sizeof(utc), "*");
    } else {
      snprintf(utc, sizeof(utc), " ");
    }
  }

  if (pmode & PMODE_VERBOSE) {
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
    snprintf(full, sizeof(full), "%s.%09ld%s", date, (long)ns, utc);
  }
  else if (pmode & PMODE_DEC)
    snprintf(full, sizeof(full), "0d%lu.%09ld%s",s,(long)ns, utc);
  else
    snprintf(full, sizeof(full), "0x%016llx%s", (unsigned long long)t, utc);

  temp = full;

  return temp;
} //tr_formatDate

/* format EvtID to a string */
std::string tr_formatActionEvent(uint64_t id, uint32_t pmode, bool json)
{
  std::stringstream full;
  string            fmt = "";
  string            extra_mark = "";
  string            extra_comma = "";
  int               width = 0;
  int               fid=0;

  fid = ((id >> 60) & 0xf);

  if (pmode & PMODE_HEX) {full << std::hex << std::setfill('0'); width = 16; fmt = "0x";}
  if (pmode & PMODE_DEC) {full << std::dec << std::setfill('0'); width = 20; fmt = "0d";}

  if (json)
  {
    extra_mark = "\"";
    extra_comma = ",";
    if (width == 0) { width = 20; } // default
  }

  if (pmode & PMODE_VERBOSE) {
    full << " " << extra_mark << "FID"   << extra_mark << ": " << fmt << std::setw(1) << ((id >> 60) & 0xf)   << extra_comma;
    full << " " << extra_mark << "GID"   << extra_mark << ": " << fmt << std::setw(4) << ((id >> 48) & 0xfff) << extra_comma;
    full << " " << extra_mark << "EVTNO" << extra_mark << ": " << fmt << std::setw(4) << ((id >> 36) & 0xfff) << extra_comma;
    if (fid == 0) {
      //full << " FLAGS: N/A";
      full << " " << extra_mark << "FLAGS" << extra_mark << ": " << extra_mark << "N/A" << extra_mark << extra_comma;
      full << " " << extra_mark << "SID"   << extra_mark << ": " << fmt << std::setw(4) << ((id >> 24) & 0xfff) << extra_comma;
      full << " " << extra_mark << "BPID"  << extra_mark << ": " << fmt << std::setw(5) << ((id >> 10) & 0x3fff) << extra_comma;
      full << " " << extra_mark << "RES"   << extra_mark << ": " << fmt << std::setw(4) << (id & 0x3ff) << extra_comma;
    } // if fid==0
    else if (fid == 1) {
      full << " " << extra_mark << "FLAGS" << extra_mark << ": " << fmt << std::setw(1) << ((id >> 32) & 0xf) << extra_comma;
      full << " " << extra_mark << "BPC"   << extra_mark << ": " << fmt << std::setw(1) << ((id >> 34) & 0x1)<< extra_comma;
      full << " " << extra_mark << "SID"   << extra_mark << ": " << fmt << std::setw(4) << ((id >> 20) & 0xfff) << extra_comma;
      full << " " << extra_mark << "BPID"  << extra_mark << ": " << fmt << std::setw(5) << ((id >> 6) & 0x3fff) << extra_comma;
      full << " " << extra_mark << "RES"   << extra_mark << ": " << fmt << std::setw(4) << (id & 0x3f) << extra_comma;
    } // if fid==1
    else {
      full << " " << extra_mark << "Other" << extra_mark << ": " << fmt << std::setw(9) << (id & 0xfffffffff) << extra_comma;
    }
  }
  else
  {
    full << " " << extra_mark << "EvtID" << extra_mark << ": " << fmt << std::setw(width) << std::setfill('0') << id << extra_comma;
  }

  return full.str();
} //tr_formatActionEvent

std::string tr_formatActionParam(uint64_t param, uint32_t evtNo, uint32_t pmode, bool json)
{
  std::stringstream full;
  string            fmt = "";
  string            extra_mark = "";
  string            extra_comma = "";
  string            extra_space = "";
  int               width = 0;

  if (pmode & PMODE_HEX) {full << std::hex << std::setfill('0'); width = 16; fmt = "0x";}
  if (pmode & PMODE_DEC) {full << std::dec << std::setfill('0'); width = 20; fmt = "0d";}

  if (json)
  {
    extra_mark = "\"";
    extra_comma = ",";
    extra_space = " ";
    if (width == 0) { width = 20; } // default
  }

  switch (evtNo) {
  case 0x0 :
    // add some code
    break;
  case 0x1 :
    // add some code
    break;
  default :
    full << " " << extra_mark << "Param" << extra_mark << ": " << fmt << std::setw(width) << param << extra_comma << extra_space << pmode;
  } // switch evtNo

  return full.str();
} // tr_formatActionParam

std::string tr_formatActionFlags(uint16_t flags, uint64_t delay, uint32_t pmode, bool json)
{
  std::stringstream full;

  full << "";

  if (json)
  {
    if (flags & 1) { full << "\"Late_ns\": " << delay << ", "; }
    else           { full << "\"Late_ns\": " << "0, "; }
    if (flags & 2) { full << "\"Early_ns\": " << -delay  << ", "; }
    else           { full << "\"Early_ns\": " << "0, "; }
    if (flags & 4) { full << "\"ConflictDelay_ns\": " << delay << ", "; }
    else           { full << "\"ConflictDelay_ns\": 0, "; }
    if (flags & 8) { full << "\"Delayed_ns\": " << delay << ", "; }
    else           { full << "\"Delayed_ns\": " << "0, "; }
  }
  else
  {
    if (flags) {
      full  << "!";
      if (flags & 1) full << "late (by " << delay << " ns)";
      if (flags & 2) full << "early (by " << -delay << " ns)";
      if (flags & 4) full << "conflict (delayed by " << delay << " ns)";
      if (flags & 8) full << "delayed (by " << delay << " ns)";
    } // if flags
  } // if json

  return full.str();
} // tr_formatActionFlags

namespace saftlib {

    int wait_for_signal(int timeout_ms) {
        return saftbus::SignalGroup::get_global().wait_for_signal(timeout_ms);
    }

}
