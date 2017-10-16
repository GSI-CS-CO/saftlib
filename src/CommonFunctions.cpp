#include "CommonFunctions.h"

using namespace std;

/* Format mask for action sink */
guint64 tr_mask(int i)
{
  return i ? (((guint64)-1) << (64-i)) : 0;
} //tr_mask

// 
std::string tr_formatDate(guint64 time, guint32 pmode)
{
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  char full[80];
  string temp;

  if (pmode & PMODE_VERBOSE) {
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
    snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns); 
  }
  else if (pmode & PMODE_DEC)
    snprintf(full, sizeof(full), "0d%lu.%09ld",s,(long)ns);
  else 
    snprintf(full, sizeof(full), "0x%016llx", (unsigned long long)time); 
  
  temp = full;
  
  return temp;
} //tr_formatDate

/* format EvtID to a string */
std::string tr_formatActionEvent(guint64 id, guint32 pmode)
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
    if (fid == 1) {
      full << " FLAGS: " << fmt << std::setw(1) << ((id >> 32) & 0xf);
      full << " SID: "   << fmt << std::setw(4) << ((id >> 20) & 0xfff);
      full << " BPID: "  << fmt << std::setw(5) << ((id >> 6) & 0x3fff);
      full << " RES: "   << fmt << std::setw(4) << (id & 0x3f);
    } // if fid==0
  }
  else full << " EvtID: " << fmt << std::setw(width) << std::setfill('0') << id;
  
  return full.str();
} //tr_formatActionEvent

std::string tr_formatActionParam(guint64 param, guint32 evtNo, guint32 pmode)
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

std::string tr_formatActionFlags(guint16 flags, guint64 delay, guint32 pmode)
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

