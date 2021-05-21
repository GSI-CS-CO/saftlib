/** Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
#include "CommonFunctions.h"

using namespace std;

/* Format mask for action sink */
uint64_t tr_mask(int i)
{
  return i ? (((uint64_t)-1) << (64-i)) : 0;
} //tr_mask

// 
std::string tr_formatDate(saftlib::Time time, uint32_t pmode)
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
std::string tr_formatActionEvent(uint64_t id, uint32_t pmode)
{
  std::stringstream full;
  string            fmt = "";
  int               width = 0;
  int               fid=0;

  fid = ((id >> 60) & 0xf);
  
  
  if (pmode & PMODE_HEX) {full << std::hex << std::setfill('0'); width = 16; fmt = "0x";}
  if (pmode & PMODE_DEC) {full << std::dec << std::setfill('0'); width = 20; fmt = "0d";}
  if (pmode & PMODE_VERBOSE) {
    full << " FID: "   << fmt << std::setw(1) << ((id >> 60) & 0xf);
    full << " GID: "   << fmt << std::setw(4) << ((id >> 48) & 0xfff);
    full << " EVTNO: " << fmt << std::setw(4) << ((id >> 36) & 0xfff);
    if (fid == 0) {
      full << " FLAGS: N/A";
      full << " SID: "   << fmt << std::setw(4) << ((id >> 24) & 0xfff);
      full << " BPID: "  << fmt << std::setw(5) << ((id >> 10) & 0x3fff);
      full << " RES: "   << fmt << std::setw(4) << (id & 0x3ff);
    } // if fid==0
    else if (fid == 1) {
      full << " FLAGS: " << fmt << std::setw(1) << ((id >> 32) & 0xf);
      full << " BPC: "   << fmt << std::setw(1) << ((id >> 34) & 0x1);
      full << " SID: "   << fmt << std::setw(4) << ((id >> 20) & 0xfff);
      full << " BPID: "  << fmt << std::setw(5) << ((id >> 6) & 0x3fff);
      full << " RES: "   << fmt << std::setw(4) << (id & 0x3f);
    } // if fid==1
    else {
      full << " Other: " << fmt << std::setw(9) << (id & 0xfffffffff);
    }
  }
  else full << " EvtID: " << fmt << std::setw(width) << std::setfill('0') << id;
  
  return full.str();
} //tr_formatActionEvent

std::string tr_formatActionParam(uint64_t param, uint32_t evtNo, uint32_t pmode)
{
  std::stringstream full;
  string fmt = "";
  int width = 0;
  
  if (pmode & PMODE_HEX) {full << std::hex << std::setfill('0'); width = 16; fmt = "0x";}
  if (pmode & PMODE_DEC) {full << std::dec << std::setfill('0'); width = 20; fmt = "0d";}
  
  switch (evtNo) {
  case 0x0 :
    // add some code
    break;
  case 0x1 :
    // add some code
    break;
  default :
    full << " Param: " << fmt << std::setw(width) << param;
  } // switch evtNo
  
  return full.str();
} // tr_formatActionParam

std::string tr_formatActionFlags(uint16_t flags, uint64_t delay, uint32_t pmode)
{
  std::stringstream full;
  
  full << "";
  
  if (flags) {
    full  << "!";
    if (flags & 1) full << "late (by " << delay << " ns)";
    if (flags & 2) full << "early (by " << -delay << " ns)";
    if (flags & 4) full << "conflict (delayed by " << delay << " ns)";
    if (flags & 8) full << "delayed (by " << delay << " ns)";
  } // if flags
  
  return full.str();
} // tr_formatActionFlags

