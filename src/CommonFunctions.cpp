#include "CommonFunctions.h"

/* Format mask for action sink */
guint64 tr_mask(int i)
{
  return i ? (((guint64)-1) << (64-i)) : 0;
}

/* Format date to a human-readable string */
const char* tr_formatDate(guint64 time)
{
  guint64 ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  static char full[80];
  
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
  snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns);

  return full;
}
